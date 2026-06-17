// Minimal Spinnaker SDK demo: enumerate cameras, open the first one,
// grab a single frame, print info, save to frame.pgm.
//
// Tested target: Chameleon3 CM3-U3-50S5C on USB3.
// Build:         see CMakeLists.txt in this directory.

#include <Spinnaker.h>
#include <SpinGenApi/SpinnakerGenApi.h>

#include <iostream>

int main() {
    Spinnaker::SystemPtr system = Spinnaker::System::GetInstance();

    const Spinnaker::LibraryVersion v = system->GetLibraryVersion();
    std::cout << "Spinnaker " << v.major << '.' << v.minor << '.'
              << v.type << '.' << v.build << '\n';

    Spinnaker::CameraList cameras = system->GetCameras();
    const unsigned n = cameras.GetSize();
    std::cout << "Cameras detected: " << n << '\n';
    if (n == 0) {
        cameras.Clear();
        system->ReleaseInstance();
        std::cerr << "No cameras found. Is the camera plugged in and is the\n"
                     "current user in the flirimaging group?\n";
        return 1;
    }

    try {
        Spinnaker::CameraPtr cam = cameras.GetByIndex(0);
        cam->Init();

        // Transport-layer node map: device identity, doesn't require Init().
        // We Init()'d already, but it's cheap.
        Spinnaker::GenApi::INodeMap& tl = cam->GetTLDeviceNodeMap();
        auto print_node = [&](const char* name) {
            Spinnaker::GenApi::CStringPtr node = tl.GetNode(name);
            if (Spinnaker::GenApi::IsReadable(node)) {
                std::cout << "  " << name << ": " << node->GetValue() << '\n';
            }
        };
        print_node("DeviceModelName");
        print_node("DeviceSerialNumber");
        print_node("DeviceVersion");

        // Configure SingleFrame so BeginAcquisition gives us one and stops.
        Spinnaker::GenApi::INodeMap& nm = cam->GetNodeMap();
        Spinnaker::GenApi::CEnumerationPtr mode = nm.GetNode("AcquisitionMode");
        Spinnaker::GenApi::CEnumEntryPtr single = mode->GetEntryByName("SingleFrame");
        mode->SetIntValue(single->GetValue());

        // Request Mono8 output from the camera instead of the default Bayer.
        Spinnaker::GenApi::CEnumerationPtr pixelFormat = nm.GetNode("PixelFormat");
        Spinnaker::GenApi::CEnumEntryPtr mono8 = pixelFormat->GetEntryByName("Mono8");
        pixelFormat->SetIntValue(mono8->GetValue());

        cam->BeginAcquisition();
        Spinnaker::ImagePtr image = cam->GetNextImage(2000);  // 2-second timeout

        if (image->IsIncomplete()) {
            std::cerr << "Image incomplete: "
                      << Spinnaker::Image::GetImageStatusDescription(image->GetImageStatus())
                      << '\n';
        } else {
            std::cout << "Captured " << image->GetWidth() << 'x' << image->GetHeight()
                      << "  format=" << image->GetPixelFormatName() << '\n';
            image->Save("frame.pgm");
            std::cout << "Wrote frame.pgm\n";
        }

        image->Release();
        cam->EndAcquisition();
        cam->DeInit();
        cam = nullptr;  // drop our smart-pointer reference before ReleaseInstance
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
