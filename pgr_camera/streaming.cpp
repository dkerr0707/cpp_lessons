// Continuous-acquisition streaming demo: pulls Mono8 frames from the first
// camera and shows them in an OpenCV window. Press 'q' or ESC in the window
// to quit.

#include <Spinnaker.h>
#include <SpinGenApi/SpinnakerGenApi.h>

#include <opencv2/core.hpp>
#include <opencv2/highgui.hpp>

#include <iostream>

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

        // Continuous acquisition.
        Spinnaker::GenApi::CEnumerationPtr mode = nm.GetNode("AcquisitionMode");
        Spinnaker::GenApi::CEnumEntryPtr continuous = mode->GetEntryByName("Continuous");
        mode->SetIntValue(continuous->GetValue());

        // Mono8 output (host doesn't need to demosaic).
        Spinnaker::GenApi::CEnumerationPtr pixelFormat = nm.GetNode("PixelFormat");
        Spinnaker::GenApi::CEnumEntryPtr mono8 = pixelFormat->GetEntryByName("Mono8");
        pixelFormat->SetIntValue(mono8->GetValue());

        cam->BeginAcquisition();
        std::cout << "Streaming. Press 'q' or ESC in the window to quit.\n";

        cv::namedWindow("camera", cv::WINDOW_NORMAL);

        while (true) {
            Spinnaker::ImagePtr image = cam->GetNextImage(1000);
            if (image->IsIncomplete()) {
                std::cerr << "Incomplete frame: "
                          << Spinnaker::Image::GetImageStatusDescription(image->GetImageStatus())
                          << '\n';
                image->Release();
                continue;
            }

            // Zero-copy wrap: cv::Mat just borrows the Spinnaker buffer.
            // imshow copies internally before returning, so it's safe to
            // Release the Spinnaker image right after.
            cv::Mat frame(static_cast<int>(image->GetHeight()),
                          static_cast<int>(image->GetWidth()),
                          CV_8UC1,
                          image->GetData(),
                          image->GetStride());
            cv::imshow("camera", frame);
            image->Release();

            const int k = cv::waitKey(1);
            if (k == 'q' || k == 27) break;  // 27 = ESC
        }

        cv::destroyAllWindows();
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
