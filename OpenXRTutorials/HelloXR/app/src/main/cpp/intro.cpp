//
// Created by jian.yang on 2024/3/29.
//
#include "common/pch.h"
#include <string.h>
#include <iostream>

#include "common/logger.h"
#include "common/common.h"
#include "openxr/openxr_platform.h"
#include "common/openxr_helper.h"

#include <android/log.h>
#include <android_native_app_glue.h>
#include <android/native_window.h>
#include <jni.h>
#include <sys/system_properties.h>

namespace Chapter1 {
//class
    class OpenXRTutorial {
    public:
        OpenXRTutorial() {
        }

        ~OpenXRTutorial() = default;

        void Run() {
            Log::Write(Log::Level::Info, "###### OpenXR Tutorial Run");
        }

    public:
        // Stored pointer to the android_app structure from android_main().
        static android_app *androidApp;

        // Custom data structure that is used by PollSystemEvents().
        // Modified from https://github.com/KhronosGroup/OpenXR-SDK-Source/blob/d6b6d7a10bdcf8d4fe806b4f415fde3dd5726878/src/tests/hello_xr/main.cpp#L133C1-L189C2
        struct AndroidAppState {
            ANativeWindow *nativeWindow = nullptr;
            bool resumed = false;
        };
        static AndroidAppState androidAppState;

        // Processes the next command from the Android OS. It updates AndroidAppState.
        static void AndroidAppHandleCmd(struct android_app *app, int32_t cmd) {
            AndroidAppState *appState = (AndroidAppState *) app->userData;

            switch (cmd) {
                // There is no APP_CMD_CREATE. The ANativeActivity creates the application thread from onCreate().
                // The application thread then calls android_main().
                case APP_CMD_START: {
                    Log::Write(Log::Level::Info, "###### APP_CMD_START");
                    break;
                }
                case APP_CMD_RESUME: {
                    appState->resumed = true;
                    break;
                }
                case APP_CMD_PAUSE: {
                    appState->resumed = false;
                    break;
                }
                case APP_CMD_STOP: {
                    break;
                }
                case APP_CMD_DESTROY: {
                    appState->nativeWindow = nullptr;
                    break;
                }
                case APP_CMD_INIT_WINDOW: {
                    appState->nativeWindow = app->window;
                    break;
                }
                case APP_CMD_TERM_WINDOW: {
                    appState->nativeWindow = nullptr;
                    break;
                }
            }
        }
        // XR_DOCS_TAG_END_Android_System_Functionality1

    private:
        void PollSystemEvents() {
            // XR_DOCS_TAG_BEGIN_Android_System_Functionality2
            // Checks whether Android has requested that application should by destroyed.
            if (androidApp->destroyRequested != 0) {
                m_applicationRunning = false;
                return;
            }
            while (true) {
                // Poll and process the Android OS system events.
                struct android_poll_source *source = nullptr;
                int events = 0;
                // The timeout depends on whether the application is active.
                const int timeoutMilliseconds = (!androidAppState.resumed && !m_sessionRunning &&
                                                 androidApp->destroyRequested == 0) ? -1 : 0;
                if (ALooper_pollAll(timeoutMilliseconds, nullptr, &events, (void **) &source) >= 0) {
                    if (source != nullptr) {
                        source->process(androidApp, source);
                    }
                } else {
                    break;
                }
            }
            // XR_DOCS_TAG_END_Android_System_Functionality2
        }

    private:
        bool m_applicationRunning = true;
        bool m_sessionRunning = false;
    };


    void OpenXRTutorial_Main() {
        Log::Write(Log::Level::Info, "###### OpenXR Tutorial Chapter 1");

        OpenXRTutorial app;
        app.Run();
    }

    android_app *OpenXRTutorial::androidApp = nullptr;
    OpenXRTutorial::AndroidAppState OpenXRTutorial::androidAppState = {};

    extern "C"
    void android_main(struct android_app *app) {
        Log::Write(Log::Level::Info, "###### main");
        JNIEnv *Env;
        app->activity->vm->AttachCurrentThread(&Env, nullptr);


        XrInstance m_xrInstance = XR_NULL_HANDLE;
        //Initialize loader
        PFN_xrInitializeLoaderKHR initializeLoader = nullptr;


        //1. instance
        OPENXR_CHECK(xrGetInstanceProcAddr(XR_NULL_HANDLE, "xrInitializeLoaderKHR",
                                           (PFN_xrVoidFunction *) &initializeLoader),
                     "Failed to get InstanceProcAddr for xrInitializeLoaderKHR.")

        if (!initializeLoader) {
            return;
        }

        XrLoaderInitInfoAndroidKHR loaderInitializeInfoAndroid{
                XR_TYPE_LOADER_INIT_INFO_ANDROID_KHR};
        loaderInitializeInfoAndroid.applicationVM = app->activity->vm;
        loaderInitializeInfoAndroid.applicationContext = app->activity->clazz;

        //2. init loader
        OPENXR_CHECK(
                initializeLoader(
                        (const XrLoaderInitInfoBaseHeaderKHR *) &loaderInitializeInfoAndroid),
                "Failed to initialize loader.")

        app->userData = &OpenXRTutorial::androidAppState;
        app->onAppCmd = OpenXRTutorial::AndroidAppHandleCmd;

        OpenXRTutorial::androidApp = app;
        OpenXRTutorial_Main();
    }
}
