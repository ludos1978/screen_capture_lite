#pragma once
#include <assert.h>
#include <chrono>
#include <cstring>
#include <functional>
#include <memory>
#include <string>
#include <thread>
#include <vector>
#include <stdio.h>

#if defined(WINDOWS) || defined(WIN32)
#if defined(SC_LITE_DLL)
#define SC_LITE_EXTERN __declspec(dllexport)
#else
#define SC_LITE_EXTERN
#endif
#else
#define SC_LITE_EXTERN
#endif


void C_RGBtoRGBA(const SL::Screen_Capture::Image &img, unsigned char *dst, size_t dst_size) {
    assert(dst_size >= static_cast<size_t>(SL::Screen_Capture::Width(img) * SL::Screen_Capture::Height(img) * sizeof(SL::Screen_Capture::ImageBGRA)));
    auto imgStart = StartSrc(img);
    auto imgWidth = Width(img);
    auto imgHeight = Height(img);

    auto imgsrc = imgStart;
    auto imgdist = dst;
    for (auto h = imgHeight-1; h >= 0 ; h--) {
        imgsrc = (SL::Screen_Capture::ImageBGRA*)(imgStart + h*img.BytesToNextRow/4);
        for (auto w = 0; w < imgWidth; w++) {
            *imgdist++ = imgsrc->R;
            *imgdist++ = imgsrc->G;
            *imgdist++ = imgsrc->B;
            *imgdist++ = 255; // alpha 255 = visible
            imgsrc++;
        }
    }
}

void C_RGBAtoRGBA(const SL::Screen_Capture::Image &img, unsigned char *dst, size_t dst_size) {
    assert(dst_size >= static_cast<size_t>(SL::Screen_Capture::Width(img) * SL::Screen_Capture::Height(img) * sizeof(SL::Screen_Capture::ImageBGRA)));
    auto imgStart = StartSrc(img);
    auto imgWidth = Width(img);
    auto imgHeight = Height(img);

    auto imgsrc = imgStart;
    auto imgdist = dst;
    for (auto h = imgHeight-1; h >= 0 ; h--) {
        imgsrc = (SL::Screen_Capture::ImageBGRA*)(imgStart + h*img.BytesToNextRow/4);
        for (auto w = 0; w < imgWidth; w++) {
            *imgdist++ = imgsrc->R;
            *imgdist++ = imgsrc->G;
            *imgdist++ = imgsrc->B;
            *imgdist++ = imgsrc->A;
            imgsrc++;
        }
    }
}

extern "C" {
namespace SL {
namespace Screen_Capture {

    // https://bravenewmethod.com/2017/10/30/unity-c-native-plugin-examples/

    std::vector<Window> cpp_windows_out;  // required, or the memory is released right after running this function
    SC_LITE_EXTERN Window* C_GetWindows (int &size) {
        cpp_windows_out = SL::Screen_Capture::GetWindows();
        size = cpp_windows_out.size();
        return cpp_windows_out.data();
    }

    std::vector<Monitor> cpp_monitor_out;  // required, or the memory is released right after running this function
    SC_LITE_EXTERN Monitor* C_GetMonitors (int &size) {
        cpp_monitor_out = SL::Screen_Capture::GetMonitors();
        size = cpp_monitor_out.size();
        return cpp_monitor_out.data();
    }

    // https://forum.unity.com/threads/returning-a-byte-array-to-c-from-objc.93503/
    SC_LITE_EXTERN typedef void ( *ImageRefWindowRefCallbackType )(const int width, const int height, const int length, unsigned char* imageData, Window* window);
    SC_LITE_EXTERN typedef void ( *ImagePtrMousePointRefCallbackType )(const int width, const int height, const int length, unsigned char* imageData, MousePoint* mPoint);

    SC_LITE_EXTERN void ImageRefWindowRefCallback( ImageRefWindowRefCallbackType fp );
    SC_LITE_EXTERN void ImagePtrMousePointRefCallback( ImagePtrMousePointRefCallbackType fp );

    static std::shared_ptr<SL::Screen_Capture::IScreenCaptureManager> frameGrabber;
    static std::shared_ptr<ICaptureConfiguration<WindowCaptureCallback>> frameGrabberConfiguration;
    
    // must be static & outside of the function (is cleared othervise?)
    static ImageRefWindowRefCallbackType frameChangedCallbackInstance;
    static ImageRefWindowRefCallbackType newFrameCallbackInstance;
    static ImagePtrMousePointRefCallbackType mouseChangedCallbackInstance;

    SC_LITE_EXTERN void C_ICaptureConfiguration (
            Window windowToCaptureInput, 
            ImageRefWindowRefCallbackType frameChangedCallback, 
            ImageRefWindowRefCallbackType newFrameCallback, 
            ImagePtrMousePointRefCallbackType mouseChangedCallback) {

        // dont assume the external variable is using the internal window variable pointers
        // so generate a new list using the windowToCaptureInput handles
        // std::vector<Window> internalWindowPointers = SL::Screen_Capture::GetWindows();
        // std::vector<Window> windowsToCapture;
        // for (int i=0; i<internalWindowPointers.size(); i++) {
        //     if (internalWindowPointers[i].Handle == windowToCaptureInput.Handle) {
        //         windowsToCapture.push_back(internalWindowPointers[i]);
        //     }
        // }
        
        frameChangedCallbackInstance = frameChangedCallback;
        newFrameCallbackInstance = newFrameCallback;
        mouseChangedCallbackInstance = mouseChangedCallback;
        
        // static std::shared_ptr<ICaptureConfiguration<WindowCaptureCallback>> frameGrabberConfiguration;
        frameGrabberConfiguration =  SL::Screen_Capture::CreateCaptureConfiguration([windowToCaptureInput]() {
            auto windows = SL::Screen_Capture::GetWindows();
            decltype(windows) filtereditems;
            for (auto &a : windows) {
                if (windowToCaptureInput.Handle == a.Handle) {
                    filtereditems.push_back(a);
                }
            }
            return filtereditems;

            // old Code
            // return windowsToCapture;

            // Example Code
            // auto windows = SL::Screen_Capture::GetWindows();
            // std::string srchterm = "blizzard";
            // // convert to lower case for easier comparisons
            // std::transform(srchterm.begin(), srchterm.end(), srchterm.begin(), [](char c) { return std::tolower(c, std::locale()); });
            // decltype(windows) filtereditems;
            // for (auto &a : windows) {
            //     std::string name = a.Name;
            //     std::transform(name.begin(), name.end(), name.begin(), [](char c) { return std::tolower(c, std::locale()); });
            //     if (name.find(srchterm) != std::string::npos) {
            //         filtereditems.push_back(a);
            //         std::cout << "ADDING WINDOW  Height " << a.Size.y << "  Width  " << a.Size.x << "   " << a.Name << std::endl;
            //     }
            // }
            // return filtereditems;            //add your own custom filtering here if you want to capture only some monitors
        });

        if (frameChangedCallbackInstance) {
            frameGrabberConfiguration->onFrameChanged([&](const SL::Screen_Capture::Image& image, const SL::Screen_Capture::Window& window) {
                if (frameChangedCallbackInstance != NULL) {
                    // Testing Callback Types
                    size_t w = image.Bounds.right - image.Bounds.left;
                    size_t h = image.Bounds.bottom - image.Bounds.top;
                    size_t pixels = w * h;
                    size_t size = pixels * sizeof(SL::Screen_Capture::ImageBGRA);
                    // write image data into this buffer
                    auto imgbuffer(std::make_unique<unsigned char[]>(size));
                    Window win = window;
                    // ::fprintf(stdout, "C_ICaptureConfiguration.onFrameChanged w:%zu h:%zu imgW:%i imgH:%i bgraSize:%zu dstSize:%zu \n\n", w, h, SL::Screen_Capture::Width(image), SL::Screen_Capture::Height(image), sizeof(SL::Screen_Capture::ImageBGRA), size);
                    // ::fprintf(stdout, "C_ICaptureConfiguration.onFrameChanged Data: %02hhx %02hhx %02hhx %02hhx %02hhx %02hhx %02hhx %02hhx\n\n", imgbuffer[0], imgbuffer[1], imgbuffer[2], imgbuffer[3], imgbuffer[4], imgbuffer[5], imgbuffer[6], imgbuffer[7]);
                    // ::fflush(stdout);
                    C_RGBtoRGBA(image, imgbuffer.get(), size);
                    frameChangedCallbackInstance(w, h, size, imgbuffer.get(), &win);
                }
            });
        }

        if (newFrameCallbackInstance) {
            frameGrabberConfiguration->onNewFrame([&](const SL::Screen_Capture::Image& image,const SL::Screen_Capture::Window& window) {
                if (newFrameCallbackInstance != NULL) {
                    // Testing Callback Types
                    size_t w = image.Bounds.right - image.Bounds.left;
                    size_t h = image.Bounds.bottom - image.Bounds.top;
                    size_t pixels = w * h;
                    size_t size = pixels * sizeof(SL::Screen_Capture::ImageBGRA);
                    auto imgbuffer(std::make_unique<unsigned char[]>(size));
                    Window win = window;
                    // ::fprintf(stdout, "C_ICaptureConfiguration.onNewFrame w:%zu h:%zu imgW:%i imgH:%i bgraSize:%zu dstSize:%zu \n\n", w, h, SL::Screen_Capture::Width(image), SL::Screen_Capture::Height(image), sizeof(SL::Screen_Capture::ImageBGRA), size);
                    // ::fflush(stdout);
                    C_RGBtoRGBA(image, imgbuffer.get(), size);
                    newFrameCallbackInstance(w, h, size, imgbuffer.get(), &win);
                }
            });
        }

        if (mouseChangedCallbackInstance) {
            frameGrabberConfiguration->onMouseChanged([&](const SL::Screen_Capture::Image* imagePtr,const SL::Screen_Capture::MousePoint& mousepoint) {
                if (mouseChangedCallbackInstance != NULL) {
                    // currently only callback when mousepos and image is received
                    if (imagePtr != nullptr) {
                        // Testing Callback Types
                        size_t w = (*imagePtr).Bounds.right - (*imagePtr).Bounds.left;
                        size_t h = (*imagePtr).Bounds.bottom - (*imagePtr).Bounds.top;
                        size_t pixels = w * h;
                        size_t size = pixels * sizeof(SL::Screen_Capture::ImageBGRA);
                        auto imgbuffer(std::make_unique<unsigned char[]>(size));
                        MousePoint mPoint = mousepoint;
                        // ::fprintf(stdout, "C_ICaptureConfiguration.onMouseChanged w:%zu h:%zu imgW:%i imgH:%i bgraSize:%zu dstSize:%zu \n\n", w, h, SL::Screen_Capture::Width(*imagePtr), SL::Screen_Capture::Height(*imagePtr), sizeof(SL::Screen_Capture::ImageBGRA), size);
                        // ::fflush(stdout);
                        C_RGBAtoRGBA(*imagePtr, imgbuffer.get(), size);
                        mouseChangedCallbackInstance(w, h, size, imgbuffer.get(), &mPoint);
                    }
                }
            });
        }

        frameGrabber = frameGrabberConfiguration->start_capturing();
    }

    SC_LITE_EXTERN void C_Capture_Start () {
    // having trouble deleting the thread & the config
    //     if (frameGrabberConfiguration != NULL) {
    //         frameGrabber = frameGrabberConfiguration->start_capturing();
    //     }
    }

    SC_LITE_EXTERN void C_Capture_Stop () {
        // if (frameGrabberConfiguration != NULL) {
        //     frameGrabberConfiguration.reset();
        //     frameGrabberConfiguration = NULL;
        // }
        frameGrabber = nullptr;
        frameGrabberConfiguration = nullptr;
        // if (frameGrabber != nullptr) {
        //     frameGrabber->abort();
        //     // TODO: how to free framegrabber?
        //     frameGrabber = nullptr;
        //     // frameGrabberConfiguration = NULL;
        // }
    }

    SC_LITE_EXTERN void C_Capture_SetFrameChangeInterval (int ms) {
        if (frameGrabber != NULL) {
            frameGrabber->setFrameChangeInterval(std::chrono::milliseconds(ms));
        }
    }

    SC_LITE_EXTERN void C_Capture_SetMouseChangeInterval (int ms) {
        if (frameGrabber != NULL) {
            frameGrabber->setMouseChangeInterval(std::chrono::milliseconds(ms));
        }
    }

    SC_LITE_EXTERN void C_Capture_Pause () {
        if (frameGrabber != NULL) {
            frameGrabber->pause();
        }
    }

    SC_LITE_EXTERN void C_Capture_Resume () {
        if (frameGrabber != NULL) {
            frameGrabber->resume();
        }
    }

    SC_LITE_EXTERN bool C_Capture_IsPaused () {
        if (frameGrabber != NULL) {
            return frameGrabber->isPaused();
        }
        return false;
    }

    SC_LITE_EXTERN bool C_Capture_ExpectedErrorEvent () {
        if (frameGrabber != NULL) {
            return frameGrabber->expectedErrorEvent();
        }
        return false;
    }


} // namespace Screen_Capture
} // namespace SL
}