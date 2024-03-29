
#include "common/pch.h"
#include "common/logger.h"
#include "common/common.h"
#include "common/options.h"
#include "common/platformdata.h"
#include "common/platformplugin.h"
#include "common/graphicsplugin.h"
#include "common/openxr_program.h"

bool UpdateOptionsFromSystemProperties(Options& options) {
#if defined(DEFAULT_GRAPHICS_PLUGIN_OPENGLES)
    options.GraphicsPlugin = "OpenGLES";
#elif defined(DEFAULT_GRAPHICS_PLUGIN_VULKAN)
    options.GraphicsPlugin = "Vulkan";
#endif

    char value[PROP_VALUE_MAX] = {};
    if (__system_property_get("debug.xr.graphicsPlugin", value) != 0) {
        options.GraphicsPlugin = value;
    }

    if (__system_property_get("debug.xr.formFactor", value) != 0) {
        options.FormFactor = value;
    }

    if (__system_property_get("debug.xr.viewConfiguration", value) != 0) {
        options.ViewConfiguration = value;
    }

    if (__system_property_get("debug.xr.blendMode", value) != 0) {
        options.EnvironmentBlendMode = value;
    }

    try {
        options.ParseStrings();
    } catch (std::invalid_argument& ia) {
        Log::Write(Log::Level::Error, ia.what());
        return false;
    }
    return true;
}

struct AndroidAppState {
    ANativeWindow* NativeWindow = nullptr;
    bool Resumed = false;
};

/**
 * Process the next main command.
 */
static void app_handle_cmd(struct android_app* app, int32_t cmd) {
    AndroidAppState* appState = (AndroidAppState*)app->userData;

    switch (cmd) {
        // There is no APP_CMD_CREATE. The ANativeActivity creates the
        // application thread from onCreate(). The application thread
        // then calls android_main().
        case APP_CMD_START: {
            Log::Write(Log::Level::Info, "    APP_CMD_START");
            Log::Write(Log::Level::Info, "onStart()");
            break;
        }
        case APP_CMD_RESUME: {
            Log::Write(Log::Level::Info, "onResume()");
            Log::Write(Log::Level::Info, "    APP_CMD_RESUME");
            appState->Resumed = true;
            break;
        }
        case APP_CMD_PAUSE: {
            Log::Write(Log::Level::Info, "onPause()");
            Log::Write(Log::Level::Info, "    APP_CMD_PAUSE");
            appState->Resumed = false;
            break;
        }
        case APP_CMD_STOP: {
            Log::Write(Log::Level::Info, "onStop()");
            Log::Write(Log::Level::Info, "    APP_CMD_STOP");
            break;
        }
        case APP_CMD_DESTROY: {
            Log::Write(Log::Level::Info, "onDestroy()");
            Log::Write(Log::Level::Info, "    APP_CMD_DESTROY");
            appState->NativeWindow = NULL;
            break;
        }
        case APP_CMD_INIT_WINDOW: {
            Log::Write(Log::Level::Info, "surfaceCreated()");
            Log::Write(Log::Level::Info, "    APP_CMD_INIT_WINDOW");
            appState->NativeWindow = app->window;
            break;
        }
        case APP_CMD_TERM_WINDOW: {
            Log::Write(Log::Level::Info, "surfaceDestroyed()");
            Log::Write(Log::Level::Info, "    APP_CMD_TERM_WINDOW");
            appState->NativeWindow = NULL;
            break;
        }
    }
}

/**
 * This is the main entry point of a native application that is using
 * android_native_app_glue.  It runs in its own thread, with its own
 * event loop for receiving input events and doing other things.
 */
void android_main(struct android_app *app) {
    Log::Write(Log::Level::Info, "###### main");
    JNIEnv *Env;
    app->activity->vm->AttachCurrentThread(&Env, nullptr);

    AndroidAppState appState = {};
    app->userData = &appState;
    app->onAppCmd = app_handle_cmd;

    std::shared_ptr<Options> options = std::make_shared<Options>();
    if (!UpdateOptionsFromSystemProperties(*options)) {
        return;
    }

    std::shared_ptr<PlatformData> data = std::make_shared<PlatformData>();
    data->applicationVM = app->activity->vm;
    data->applicationActivity = app->activity->clazz;

    std::shared_ptr<IPlatformPlugin> platformPlugin = CreatePlatformPlugin(options, data);
    Log::Write(Log::Level::Info,"###### 1.CreatePlatformPlugin");
    // Create graphics API implementation.
    std::shared_ptr<IGraphicsPlugin> graphicsPlugin = CreateGraphicsPlugin(options, platformPlugin);
    Log::Write(Log::Level::Info,"###### 2.CreateGraphicsPlugin");
    // Initialize the OpenXR program.
    std::shared_ptr<IOpenXrProgram> program = CreateOpenXrProgram(options, platformPlugin, graphicsPlugin);
    Log::Write(Log::Level::Info,"###### 3.CreateOpenXrProgram");

    // Initialize the loader for this platform
    PFN_xrInitializeLoaderKHR initializeLoader = nullptr;
    if (XR_SUCCEEDED(
            xrGetInstanceProcAddr(XR_NULL_HANDLE, "xrInitializeLoaderKHR", (PFN_xrVoidFunction*)(&initializeLoader)))) {
        XrLoaderInitInfoAndroidKHR loaderInitInfoAndroid = {XR_TYPE_LOADER_INIT_INFO_ANDROID_KHR};
        loaderInitInfoAndroid.applicationVM = app->activity->vm;
        loaderInitInfoAndroid.applicationContext = app->activity->clazz;
        initializeLoader((const XrLoaderInitInfoBaseHeaderKHR*)&loaderInitInfoAndroid);
    }

    program->CreateInstance();
    program->InitializeSystem();

    options->SetEnvironmentBlendMode(program->GetPreferredBlendMode());
    UpdateOptionsFromSystemProperties(*options);
    platformPlugin->UpdateOptions(options);
    graphicsPlugin->UpdateOptions(options);

    program->InitializeDevice();
    program->InitializeSession();
    program->CreateSwapchains();

    bool requestRestart = false;
    bool exitRenderLoop = false;

    while (app->destroyRequested == 0) {
        // Read all pending events.
        for (;;) {
            int events;
            struct android_poll_source* source;
            // If the timeout is zero, returns immediately without blocking.
            // If the timeout is negative, waits indefinitely until an event appears.
            const int timeoutMilliseconds =
                    (!appState.Resumed && !program->IsSessionRunning() && app->destroyRequested == 0) ? -1 : 0;
            if (ALooper_pollAll(timeoutMilliseconds, nullptr, &events, (void**)&source) < 0) {
                break;
            }

            // Process this event.
            if (source != nullptr) {
                source->process(app, source);
            }
        }

        program->PollEvents(&exitRenderLoop, &requestRestart);
        if (exitRenderLoop) {
            ANativeActivity_finish(app->activity);
            continue;
        }

        if (!program->IsSessionRunning()) {
            // Throttle loop since xrWaitFrame won't be called.
            std::this_thread::sleep_for(std::chrono::milliseconds(250));
            continue;
        }

        program->PollActions();
        program->RenderFrame();
    }

    app->activity->vm->DetachCurrentThread();
}