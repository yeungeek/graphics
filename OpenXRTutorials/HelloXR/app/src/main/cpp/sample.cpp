//
// Created by jian.yang on 2024/4/11.
//

// For Android Platform
#include <string>
#include <android/log.h>
#include <android_native_app_glue.h>
#include <android/native_window.h>
#include <jni.h>
#include <sys/system_properties.h>
#include <EGL/egl.h>

#include "openxr/openxr_platform.h"
#include "common/openxr_helper.h"
#include "common/common.h"

#define LOG_TAG "sample"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGW(...) __android_log_print(ANDROID_LOG_WARN, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

namespace Sample {
    // properties
    void* applicationVM;
    void* applicationActivity;

    // openxr
    XrInstanceCreateInfoAndroidKHR instanceCreateInfoAndroid;
    XrInstance m_xrInstance{XR_NULL_HANDLE};
    XrSystemId m_systemId{XR_NULL_SYSTEM_ID};

    struct AndroidAppState {
        ANativeWindow *NativeWindow = nullptr;
        bool Resumed = false;
    };

    /**
     * Process the next main command.
     */
    static void app_handle_cmd(struct android_app *app, int32_t cmd) {

    }

    /**
     * log
     */


    /**
     * logic
     */
    void create_android_instance() {
        instanceCreateInfoAndroid ={XR_TYPE_INSTANCE_CREATE_INFO_ANDROID_KHR};
        instanceCreateInfoAndroid.applicationVM = applicationVM;
        instanceCreateInfoAndroid.applicationActivity = applicationActivity;
    }

    void create_openxr_instance(){
        LOGI("###### create openxr instance");
        std::vector<const char*> extensions;
        const std::vector<std::string> platformExtensions = {XR_KHR_ANDROID_CREATE_INSTANCE_EXTENSION_NAME};
        std::transform(platformExtensions.begin(), platformExtensions.end(), std::back_inserter(extensions),
                       [](const std::string& ext) { return ext.c_str(); });

        const std::vector<std::string> graphicsExtensions = {XR_KHR_OPENGL_ES_ENABLE_EXTENSION_NAME};
        std::transform(graphicsExtensions.begin(), graphicsExtensions.end(), std::back_inserter(extensions),
                       [](const std::string& ext) { return ext.c_str(); });

        XrInstanceCreateInfo createInfo{XR_TYPE_INSTANCE_CREATE_INFO};
        createInfo.next = (XrBaseInStructure*)&instanceCreateInfoAndroid;
        createInfo.enabledExtensionCount = (uint32_t)extensions.size();
        createInfo.enabledExtensionNames = extensions.data();

        strcpy(createInfo.applicationInfo.applicationName, "OpenXR Sample");
        createInfo.applicationInfo.apiVersion = XR_CURRENT_API_VERSION;
        OPENXR_CHECK(xrCreateInstance(&createInfo, &m_xrInstance), "Failed to create OpenXR instance.")
    }

    void initialize_system() {
        XrSystemGetInfo systemInfo{XR_TYPE_SYSTEM_GET_INFO};
        systemInfo.formFactor = XR_FORM_FACTOR_HEAD_MOUNTED_DISPLAY;
        OPENXR_CHECK(xrGetSystem(m_xrInstance, &systemInfo, &m_systemId), "Failed to get system info.")
        LOGI("###### Using system %d for form factor %s",m_systemId,to_string(systemInfo.formFactor));
    }

    extern "C"
    void android_main(struct android_app *app) {
        JNIEnv *Env;
        app->activity->vm->AttachCurrentThread(&Env, nullptr);

        AndroidAppState appState = {};
        app->userData = &appState;
        app->onAppCmd = app_handle_cmd;

        applicationVM = app->activity->vm;
        applicationActivity = app->activity->clazz;

        //1. initialize
        //2. loop
        //3. destroy
        LOGI("###### 1. initialize");
        create_android_instance();


        // Initialize the loader
        PFN_xrInitializeLoaderKHR  initializeLoader = nullptr;
        OPENXR_CHECK(xrGetInstanceProcAddr(XR_NULL_HANDLE, "xrInitializeLoaderKHR",
                                           (PFN_xrVoidFunction *) &initializeLoader),
                     "Failed to get InstanceProcAddr for xrInitializeLoaderKHR.")

        if(!initializeLoader) {
            LOGE("Failed to get InstanceProcAddr for xrInitializeLoaderKHR.");
            return;
        }

        XrLoaderInitInfoAndroidKHR loaderInitInfoAndroid = {XR_TYPE_LOADER_INIT_INFO_ANDROID_KHR};
        loaderInitInfoAndroid.applicationVM = app->activity->vm;
        loaderInitInfoAndroid.applicationContext = app->activity->clazz;
        initializeLoader((const XrLoaderInitInfoBaseHeaderKHR*)&loaderInitInfoAndroid);

        // Create OpenXR Instance
        create_openxr_instance();
        initialize_system();

    }
}