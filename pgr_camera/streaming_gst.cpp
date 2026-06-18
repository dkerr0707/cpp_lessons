// Streaming demo via GStreamer: pulls Mono8 frames and pushes them into an
// appsrc -> videoconvert -> autovideosink pipeline. Ctrl-C to quit.
//
// Frames are zero-copy: we wrap the Spinnaker buffer in a GstBuffer with a
// destroy callback that releases the ImagePtr once the pipeline is done with
// the data.

#include <Spinnaker.h>
#include <SpinGenApi/SpinnakerGenApi.h>

#include <gst/gst.h>
#include <gst/app/gstappsrc.h>

#include <atomic>
#include <chrono>
#include <csignal>
#include <iostream>

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
        pf->SetIntValue(pf->GetEntryByName("Mono8")->GetValue());

        Spinnaker::GenApi::CIntegerPtr widthNode = nm.GetNode("Width");
        Spinnaker::GenApi::CIntegerPtr heightNode = nm.GetNode("Height");
        const int width = static_cast<int>(widthNode->GetValue());
        const int height = static_cast<int>(heightNode->GetValue());

        pipeline = gst_parse_launch(
            "appsrc name=src is-live=true block=true format=time "
            "! videoconvert "
            "! autovideosink sync=false",
            nullptr);
        if (!pipeline) {
            std::cerr << "Failed to build GStreamer pipeline\n";
            cameras.Clear();
            system->ReleaseInstance();
            return 2;
        }

        appsrc = GST_APP_SRC(gst_bin_get_by_name(GST_BIN(pipeline), "src"));
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
                  << " GRAY8 via GStreamer. Ctrl-C to quit.\n";

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
