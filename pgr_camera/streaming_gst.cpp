// Streaming demo via GStreamer with GPU demosaicing.
//
// Camera ships raw BayerRG8. We upload that to the GPU as a single-channel
// GRAY8 texture, then a custom GLSL fragment shader does a bilinear bayer
// demosaic. glimagesink displays the result without any GL->host round-trip.
//
// Frames are zero-copy: we wrap the Spinnaker buffer in a GstBuffer with a
// destroy callback that releases the ImagePtr once the pipeline is done with
// the data.
//
// Pipeline:
//   appsrc (video/x-raw GRAY8)
//     -> glupload                 (CPU memory -> GL texture)
//     -> glcolorconvert           (R8 -> RGBA on the GPU; glshader requires RGBA)
//     -> glshader (demosaic)      (custom GLSL bayer -> RGB)
//     -> glcolorscale             (GPU-side downscale to half resolution)
//     -> glimagesink              (zero-copy GL display)
//
// Ctrl-C to quit.

#include <Spinnaker.h>
#include <SpinGenApi/SpinnakerGenApi.h>

#include <gst/gst.h>
#include <gst/app/gstappsrc.h>
#include <gst/video/video.h>

#include <atomic>
#include <chrono>
#include <csignal>
#include <iostream>
#include <string>

namespace {

std::atomic<bool> stop_requested{false};

void on_sigint(int) { stop_requested = true; }

// Holder kept alive while GStreamer owns the buffer; deleted by the destroy
// callback below, which also returns the frame to the Spinnaker buffer pool.
struct ImageHolder {
    Spinnaker::ImagePtr image;
};

void release_image(gpointer user_data) {
    auto* holder = static_cast<ImageHolder*>(user_data);
    holder->image->Release();
    delete holder;
}

}  // namespace

int main(int argc, char* argv[]) {
    gst_init(&argc, &argv);
    std::signal(SIGINT, on_sigint);

    Spinnaker::SystemPtr system = Spinnaker::System::GetInstance();
    Spinnaker::CameraList cameras = system->GetCameras();
    if (cameras.GetSize() == 0) {
        cameras.Clear();
        system->ReleaseInstance();
        std::cerr << "No cameras found.\n";
        return 1;
    }

    int rc = 0;
    GstElement* pipeline = nullptr;
    GstAppSrc* appsrc = nullptr;

    try {
        Spinnaker::CameraPtr cam = cameras.GetByIndex(0);
        cam->Init();

        Spinnaker::GenApi::INodeMap& nm = cam->GetNodeMap();

        Spinnaker::GenApi::CEnumerationPtr mode = nm.GetNode("AcquisitionMode");
        mode->SetIntValue(mode->GetEntryByName("Continuous")->GetValue());

        Spinnaker::GenApi::CEnumerationPtr pf = nm.GetNode("PixelFormat");
        pf->SetIntValue(pf->GetEntryByName("BayerRG8")->GetValue());

        // Continuous auto white balance — camera adjusts per-channel gains
        // live based on scene content. Applied to the raw bayer samples
        // before they leave the camera, so our demosaic shader benefits.
        Spinnaker::GenApi::CEnumerationPtr awb = nm.GetNode("BalanceWhiteAuto");
        if (awb.IsValid() && Spinnaker::GenApi::IsWritable(awb)) {
            Spinnaker::GenApi::CEnumEntryPtr cont = awb->GetEntryByName("Continuous");
            if (cont.IsValid() && Spinnaker::GenApi::IsAvailable(cont)) {
                awb->SetIntValue(cont->GetValue());
                std::cout << "Auto white balance: Continuous\n";
            }
        }

        Spinnaker::GenApi::CIntegerPtr widthNode = nm.GetNode("Width");
        Spinnaker::GenApi::CIntegerPtr heightNode = nm.GetNode("Height");
        const int width = static_cast<int>(widthNode->GetValue());
        const int height = static_cast<int>(heightNode->GetValue());

        const int display_w = width / 2;
        const int display_h = height / 2;
        const std::string pipeline_desc =
            "appsrc name=src is-live=true block=true format=time "
            "! glupload "
            "! glcolorconvert "
            "! glshader name=demosaic "
            "! glcolorscale "
            "! video/x-raw(memory:GLMemory),format=RGBA,width=" +
            std::to_string(display_w) + ",height=" +
            std::to_string(display_h) +
            " ! glimagesink sync=false";
        pipeline = gst_parse_launch(pipeline_desc.c_str(), nullptr);
        if (!pipeline) {
            std::cerr << "Failed to build GStreamer pipeline\n";
            cameras.Clear();
            system->ReleaseInstance();
            return 2;
        }

        // Bilinear bayer (RGGB) -> RGB demosaic. The texture is single-channel
        // GRAY8 — we treat each texel as a raw bayer sample and reconstruct
        // R/G/B from the pixel's position in the 2x2 pattern.
        static const char* kBayerShader = R"GLSL(
            #version 150
            in vec2 v_texcoord;
            out vec4 fragColor;
            uniform sampler2D tex;

            void main() {
                ivec2 sz   = textureSize(tex, 0);
                ivec2 ipix = ivec2(v_texcoord * vec2(sz));
                int   xp   = ipix.x & 1;
                int   yp   = ipix.y & 1;

                // texelFetch: unfiltered integer-coordinate access. Avoids
                // any blending across adjacent bayer samples that would
                // destroy the color pattern.
                float c  = texelFetch(tex, ipix,                  0).r;
                float n  = texelFetch(tex, ipix + ivec2( 0, -1),  0).r;
                float s  = texelFetch(tex, ipix + ivec2( 0,  1),  0).r;
                float e  = texelFetch(tex, ipix + ivec2( 1,  0),  0).r;
                float w  = texelFetch(tex, ipix + ivec2(-1,  0),  0).r;
                float ne = texelFetch(tex, ipix + ivec2( 1, -1),  0).r;
                float nw = texelFetch(tex, ipix + ivec2(-1, -1),  0).r;
                float se = texelFetch(tex, ipix + ivec2( 1,  1),  0).r;
                float sw = texelFetch(tex, ipix + ivec2(-1,  1),  0).r;

                // RGGB pattern (matches Spinnaker's BayerRG8):
                //   (0,0) = R, (1,0) = G_on_R_row, (0,1) = G_on_B_row, (1,1) = B
                vec3 rgb;
                if (xp == 0 && yp == 0) {              // R pixel
                    rgb = vec3(c, (n+s+e+w)*0.25, (ne+nw+se+sw)*0.25);
                } else if (xp == 1 && yp == 0) {       // G on R-row
                    rgb = vec3((e+w)*0.5, c, (n+s)*0.5);
                } else if (xp == 0 && yp == 1) {       // G on B-row
                    rgb = vec3((n+s)*0.5, c, (e+w)*0.5);
                } else {                                // B pixel
                    rgb = vec3((ne+nw+se+sw)*0.25, (n+s+e+w)*0.25, c);
                }

                fragColor = vec4(rgb, 1.0);
            }
        )GLSL";

        GstElement* shader = gst_bin_get_by_name(GST_BIN(pipeline), "demosaic");
        g_object_set(shader, "fragment", kBayerShader, nullptr);
        gst_object_unref(shader);

        appsrc = GST_APP_SRC(gst_bin_get_by_name(GST_BIN(pipeline), "src"));
        // We lie to glupload about the format: bayer bytes uploaded as a
        // single-channel grayscale texture, then the shader reinterprets.
        GstCaps* caps = gst_caps_new_simple(
            "video/x-raw",
            "format",    G_TYPE_STRING,      "GRAY8",
            "width",     G_TYPE_INT,         width,
            "height",    G_TYPE_INT,         height,
            "framerate", GST_TYPE_FRACTION,  30, 1,
            nullptr);
        gst_app_src_set_caps(appsrc, caps);
        gst_caps_unref(caps);

        cam->BeginAcquisition();
        gst_element_set_state(pipeline, GST_STATE_PLAYING);
        std::cout << "Streaming " << width << 'x' << height
                  << " BayerRG8 -> GPU GLSL demosaic, displayed at "
                  << display_w << 'x' << display_h
                  << ". Ctrl-C to quit.\n";

        const auto t0 = std::chrono::steady_clock::now();

        while (!stop_requested) {
            Spinnaker::ImagePtr image = cam->GetNextImage(1000);
            if (image->IsIncomplete()) {
                image->Release();
                continue;
            }

            const std::size_t size = image->GetBufferSize();
            auto* holder = new ImageHolder{image};

            GstBuffer* buffer = gst_buffer_new_wrapped_full(
                static_cast<GstMemoryFlags>(0),
                image->GetData(),
                size,
                0,           // offset
                size,        // valid bytes
                holder,
                release_image);

            // Tell downstream (especially glupload) the frame layout. Without
            // this it tries to negotiate a buffer pool with assumed defaults
            // and the pool allocation fails.
            gst_buffer_add_video_meta(buffer,
                                      GST_VIDEO_FRAME_FLAG_NONE,
                                      GST_VIDEO_FORMAT_GRAY8,
                                      width, height);

            const auto now = std::chrono::steady_clock::now();
            GST_BUFFER_PTS(buffer) =
                std::chrono::duration_cast<std::chrono::nanoseconds>(now - t0).count();
            GST_BUFFER_DURATION(buffer) = GST_SECOND / 30;

            const GstFlowReturn ret = gst_app_src_push_buffer(appsrc, buffer);
            // Ownership of `buffer` transfers to appsrc on success; the
            // destroy callback releases the Spinnaker image when GStreamer
            // is done with it.
            if (ret != GST_FLOW_OK) {
                std::cerr << "Push buffer failed: " << ret << '\n';
                break;
            }
        }

        std::cout << "\nShutting down...\n";
        cam->EndAcquisition();
        gst_app_src_end_of_stream(appsrc);
        gst_element_set_state(pipeline, GST_STATE_NULL);
        gst_object_unref(appsrc);
        gst_object_unref(pipeline);

        cam->DeInit();
        cam = nullptr;
    }
    catch (Spinnaker::Exception& e) {
        std::cerr << "Spinnaker error: " << e.what() << '\n';
        rc = 2;
    }

    cameras.Clear();
    system->ReleaseInstance();
    gst_deinit();
    return rc;
}
