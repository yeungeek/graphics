
#include "common/pch.h"
#include "common/logger.h"
#include "common/common.h"
#include "common/options.h"

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

    // Initialize the loader for this platform
    PFN_xrInitializeLoaderKHR initializeLoader = nullptr;
}