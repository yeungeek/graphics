//
// Created by jian.yang on 2024/3/6.
//

#include <DebugOutput.h>
#include <GraphicsAPI_OpenGL_ES.h>
#include <OpenXRDebugUtils.h>

class OpenXRTutorial {
public:
    OpenXRTutorial(GraphicsAPI_Type apiType) {
        XR_TUT_LOG("###### OpenXRTutorial")
    }

    ~OpenXRTutorial() {
        XR_TUT_LOG("###### OpenXRTutorial::~OpenXRTutorial")
    }

    void Run() {

    }

public:
    static android_app *androidApp;
    // Custom data structure that is used by PollSystemEvents().
    struct AndroidAppState {
        ANativeWindow *nativeWindow = nullptr;
        bool resumed = false;
    };
    static AndroidAppState androidAppState;

    static void AndroidAppHandleCmd(struct android_app *app, int32_t cmd) {
        AndroidAppState *appState = (AndroidAppState *) app->userData;
        switch (cmd) {
            case APP_CMD_START: XR_TUT_LOG("APP_CMD_START")
                break;
            case APP_CMD_RESUME:
                appState->resumed = true;
                break;
            case APP_CMD_PAUSE:
                appState->resumed = false;
                break;
            case APP_CMD_STOP: XR_TUT_LOG("APP_CMD_STOP")
                break;
            case APP_CMD_DESTROY: XR_TUT_LOG("APP_CMD_DESTROY")
                appState->nativeWindow = nullptr;
                break;
            case APP_CMD_INIT_WINDOW: XR_TUT_LOG("APP_CMD_INIT_WINDOW")
                appState->nativeWindow = app->window;
                break;
            case APP_CMD_TERM_WINDOW: XR_TUT_LOG("APP_CMD_TERM_WINDOW")
                appState->nativeWindow = nullptr;
        }
    }

private:
    void PollSystemEvents() {
        XR_TUT_LOG("###### PollSystemEvents")
        if (androidApp->destroyRequested != 0) {
            m_applicationRunning = false;
            return;
        }
        while (true) {
            struct android_poll_source *source = nullptr;
            int events = 0;

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
    }

private:
    bool m_applicationRunning = true;
    bool m_sessionRunning = false;
};

void OpenXRTutorialMain(GraphicsAPI_Type apiType) {
    XR_TUT_LOG("###### OpenXRTutorialMain")
    DebugOutput debugOutput;
    OpenXRTutorial app(apiType);
    app.Run();
}

android_app *OpenXRTutorial::androidApp = nullptr;
OpenXRTutorial::AndroidAppState OpenXRTutorial::androidAppState = {};

void android_main(struct android_app *app) {
    XR_TUT_LOG("###### android main")
    JNIEnv *env;
    app->activity->vm->AttachCurrentThread(&env, nullptr);

    // Load xrInitializeLoaderKHR() function pointer. On Android, the loader must be initialized with variables from android_app *.
    XrInstance m_xrInstance = XR_NULL_HANDLE;
    PFN_xrInitializeLoaderKHR xrInitializeLoaderKHR = nullptr;
    OPENXR_CHECK(xrGetInstanceProcAddr(XR_NULL_HANDLE, "xrInitializeLoaderKHR",
                                       (PFN_xrVoidFunction *) &xrInitializeLoaderKHR),
                 "Failed to xrGetInstanceProcAddr");

    // Fill out an XrLoaderInitInfoAndroidKHR structure and initialize the loader for Android.
    if (!xrInitializeLoaderKHR) {
        return;
    }

    XrLoaderInitInfoAndroidKHR loaderInitializeInfoAndroid{XR_TYPE_LOADER_INIT_INFO_ANDROID_KHR};
    loaderInitializeInfoAndroid.applicationVM = app->activity->vm;
    loaderInitializeInfoAndroid.applicationContext = app->activity->clazz;
    OPENXR_CHECK(
            xrInitializeLoaderKHR((XrLoaderInitInfoBaseHeaderKHR *) &loaderInitializeInfoAndroid),
            "Failed to initialize Loader for Android");

    app->userData = &OpenXRTutorial::androidAppState;
    app->onAppCmd = OpenXRTutorial::AndroidAppHandleCmd;

    OpenXRTutorial::androidApp = app;

    OpenXRTutorialMain(XR_TUTORIAL_GRAPHICS_API);
}