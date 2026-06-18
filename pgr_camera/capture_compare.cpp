// Capture two frames of the same scene:
//   frame_rgb.png    -- RGB8 with camera-side demosaic + AWB
//   frame_bayer.pgm  -- raw BayerRG8 mosaic
//
// Lets us compare what colors the camera *thinks* it's seeing vs. what our
// streaming shader produces from the raw mosaic.

#include <Spinnaker.h>
#include <SpinGenApi/SpinnakerGenApi.h>

#include <iostream>

namespace {

void set_enum(Spinnaker::GenApi::INodeMap& nm, const char* node, const char* entry) {
    Spinnaker::GenApi::CEnumerationPtr e = nm.GetNode(node);
    e->SetIntValue(e->GetEntryByName(entry)->GetValue());
}

}  // namespace

int main() {
    Spinnaker::SystemPtr system = Spinnaker::System::GetInstance();
    Spinnaker::CameraList cameras = system->GetCameras();
    if (cameras.GetSize() == 0) {
        cameras.Clear();
        system->ReleaseInstance();
        std::cerr << "No cameras found.\n";
        return 1;
    }

    try {
        Spinnaker::CameraPtr cam = cameras.GetByIndex(0);
        cam->Init();
        Spinnaker::GenApi::INodeMap& nm = cam->GetNodeMap();

        set_enum(nm, "AcquisitionMode", "Continuous");

        // --- RGB8 with continuous AWB ---
        set_enum(nm, "PixelFormat", "RGB8");
        set_enum(nm, "BalanceWhiteAuto", "Continuous");

        cam->BeginAcquisition();

        // Burn ~30 frames so AWB converges.
        for (int i = 0; i < 30; ++i) {
            Spinnaker::ImagePtr im = cam->GetNextImage(1000);
            im->Release();
        }
        {
            Spinnaker::ImagePtr im = cam->GetNextImage(1000);
            std::cout << "RGB8: " << im->GetWidth() << 'x' << im->GetHeight()
                      << " format=" << im->GetPixelFormatName() << '\n';
            im->Save("frame_rgb.png");
            std::cout << "  wrote frame_rgb.png\n";
            im->Release();
        }
        cam->EndAcquisition();

        // --- BayerRG8 raw ---
        set_enum(nm, "PixelFormat", "BayerRG8");
        cam->BeginAcquisition();
        for (int i = 0; i < 10; ++i) {
            Spinnaker::ImagePtr im = cam->GetNextImage(1000);
            im->Release();
        }
        {
            Spinnaker::ImagePtr im = cam->GetNextImage(1000);
            std::cout << "Bayer: " << im->GetWidth() << 'x' << im->GetHeight()
                      << " format=" << im->GetPixelFormatName() << '\n';
            im->Save("frame_bayer.pgm");
            std::cout << "  wrote frame_bayer.pgm\n";
            im->Release();
        }
        cam->EndAcquisition();

        cam->DeInit();
        cam = nullptr;
    }
    catch (Spinnaker::Exception& e) {
        std::cerr << "Spinnaker error: " << e.what() << '\n';
        cameras.Clear();
        system->ReleaseInstance();
        return 2;
    }

    cameras.Clear();
    system->ReleaseInstance();
    return 0;
}
