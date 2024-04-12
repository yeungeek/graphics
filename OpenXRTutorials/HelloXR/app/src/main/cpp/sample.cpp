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

// system
#include <array>

#include "openxr/openxr_platform.h"
#include "common/openxr_helper.h"
#include "common/gfxwrapper_opengl.h"
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
    XrSession m_session{XR_NULL_HANDLE};
    XrSystemId m_systemId{XR_NULL_SYSTEM_ID};
    XrSpace m_appSpace{XR_NULL_HANDLE};

    // config
    XrViewConfigurationType viewConfigType{XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO};
    XrEnvironmentBlendMode environmentBlendMode{XR_ENVIRONMENT_BLEND_MODE_OPAQUE};

    XrGraphicsBindingOpenGLESAndroidKHR m_graphicsBinding{XR_TYPE_GRAPHICS_BINDING_OPENGL_ES_ANDROID_KHR};

    std::vector<XrSpace> m_visualizedSpaces;
    // color
    std::array<float, 4> m_clearColor = {0.184313729f, 0.309803933f, 0.309803933f, 1.0f};

    ksGpuWindow window{};

    GLint m_contextApiMajorVersion{0};

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
     * utils
     */
    namespace Math {
        namespace Pose {
            XrPosef Identity() {
                XrPosef t{};
                t.orientation.w = 1;
                return t;
            }

            XrPosef Translation(const XrVector3f& translation) {
                XrPosef t = Identity();
                t.position = translation;
                return t;
            }

            XrPosef RotateCCWAboutYAxis(float radians, XrVector3f translation) {
                XrPosef t = Identity();
                t.orientation.x = 0.f;
                t.orientation.y = std::sin(radians * 0.5f);
                t.orientation.z = 0.f;
                t.orientation.w = std::cos(radians * 0.5f);
                t.position = translation;
                return t;
            }
        }  // namespace Pose
    }  // namespace Math

    inline XrReferenceSpaceCreateInfo GetXrReferenceSpaceCreateInfo(const std::string& referenceSpaceTypeStr) {
        XrReferenceSpaceCreateInfo referenceSpaceCreateInfo{XR_TYPE_REFERENCE_SPACE_CREATE_INFO};
        referenceSpaceCreateInfo.poseInReferenceSpace = Math::Pose::Identity();
        if (EqualsIgnoreCase(referenceSpaceTypeStr, "View")) {
            referenceSpaceCreateInfo.referenceSpaceType = XR_REFERENCE_SPACE_TYPE_VIEW;
        } else if (EqualsIgnoreCase(referenceSpaceTypeStr, "ViewFront")) {
            // Render head-locked 2m in front of device.
            referenceSpaceCreateInfo.poseInReferenceSpace = Math::Pose::Translation({0.f, 0.f, -2.f}),
                    referenceSpaceCreateInfo.referenceSpaceType = XR_REFERENCE_SPACE_TYPE_VIEW;
        } else if (EqualsIgnoreCase(referenceSpaceTypeStr, "Local")) {
            referenceSpaceCreateInfo.referenceSpaceType = XR_REFERENCE_SPACE_TYPE_LOCAL;
        } else if (EqualsIgnoreCase(referenceSpaceTypeStr, "Stage")) {
            referenceSpaceCreateInfo.referenceSpaceType = XR_REFERENCE_SPACE_TYPE_STAGE;
        } else if (EqualsIgnoreCase(referenceSpaceTypeStr, "StageLeft")) {
            referenceSpaceCreateInfo.poseInReferenceSpace = Math::Pose::RotateCCWAboutYAxis(0.f, {-2.f, 0.f, -2.f});
            referenceSpaceCreateInfo.referenceSpaceType = XR_REFERENCE_SPACE_TYPE_STAGE;
        } else if (EqualsIgnoreCase(referenceSpaceTypeStr, "StageRight")) {
            referenceSpaceCreateInfo.poseInReferenceSpace = Math::Pose::RotateCCWAboutYAxis(0.f, {2.f, 0.f, -2.f});
            referenceSpaceCreateInfo.referenceSpaceType = XR_REFERENCE_SPACE_TYPE_STAGE;
        } else if (EqualsIgnoreCase(referenceSpaceTypeStr, "StageLeftRotated")) {
            referenceSpaceCreateInfo.poseInReferenceSpace = Math::Pose::RotateCCWAboutYAxis(3.14f / 3.f, {-2.f, 0.5f, -2.f});
            referenceSpaceCreateInfo.referenceSpaceType = XR_REFERENCE_SPACE_TYPE_STAGE;
        } else if (EqualsIgnoreCase(referenceSpaceTypeStr, "StageRightRotated")) {
            referenceSpaceCreateInfo.poseInReferenceSpace = Math::Pose::RotateCCWAboutYAxis(-3.14f / 3.f, {2.f, 0.5f, -2.f});
            referenceSpaceCreateInfo.referenceSpaceType = XR_REFERENCE_SPACE_TYPE_STAGE;
        } else {
            throw std::invalid_argument(Fmt("Unknown reference space type '%s'", referenceSpaceTypeStr.c_str()));
        }
        return referenceSpaceCreateInfo;
    }

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

    void create_visualized_spaces() {
        LOGI("###### create visualized spaces");
        std::string visualizedSpaces[] = {"ViewFront",        "Local", "Stage", "StageLeft", "StageRight", "StageLeftRotated",
                                          "StageRightRotated"};
        for(const auto& visualizedSpace : visualizedSpaces){
            XrReferenceSpaceCreateInfo referenceSpaceCreateInfo = GetXrReferenceSpaceCreateInfo(visualizedSpace);
            XrSpace space;
            XrResult  res = xrCreateReferenceSpace(m_session,&referenceSpaceCreateInfo,&space);

            if (XR_SUCCEEDED(res)) {
                m_visualizedSpaces.push_back(space);
            } else {
                LOGE("###### Failed to create reference space %s with error %d", visualizedSpace.c_str(),res);
            }
        }
    }

    void initialize_system() {
        XrSystemGetInfo systemInfo{XR_TYPE_SYSTEM_GET_INFO};
        systemInfo.formFactor = XR_FORM_FACTOR_HEAD_MOUNTED_DISPLAY;
        OPENXR_CHECK(xrGetSystem(m_xrInstance, &systemInfo, &m_systemId), "Failed to get system info.")
        LOGI("###### Using system %d for form factor %s",m_systemId,to_string(systemInfo.formFactor));
    }

    void initialize_device(){
        // Extension function must be loaded by name
        LOGI("###### initialize device");
        PFN_xrGetOpenGLESGraphicsRequirementsKHR pfnGetOpenGLESGraphicsRequirementsKHR  = nullptr;
        OPENXR_CHECK(xrGetInstanceProcAddr(m_xrInstance, "xrGetOpenGLESGraphicsRequirementsKHR",
                                                                 reinterpret_cast<PFN_xrVoidFunction*>(&pfnGetOpenGLESGraphicsRequirementsKHR)),
                     "Failed to get InstanceProcAddr for xrGetOpenGLESGraphicsRequirementsKHR.")

        XrGraphicsRequirementsOpenGLESKHR graphicsRequirements{XR_TYPE_GRAPHICS_REQUIREMENTS_OPENGL_ES_KHR};
        OPENXR_CHECK(pfnGetOpenGLESGraphicsRequirementsKHR(m_xrInstance, m_systemId, &graphicsRequirements),
                     "Failed to get OpenGL ES graphics requirements.")

        // Initialize gl extension
        ksDriverInstance driverInstance{};
        ksGpuQueueInfo queueInfo{};
        ksGpuSurfaceColorFormat colorFormat{KS_GPU_SURFACE_COLOR_FORMAT_B8G8R8A8};
        ksGpuSurfaceDepthFormat depthFormat{KS_GPU_SURFACE_DEPTH_FORMAT_D24};
        ksGpuSampleCount  sampleCount{KS_GPU_SAMPLE_COUNT_1};
        if(!ksGpuWindow_Create(&window, &driverInstance, &queueInfo, 0,colorFormat, depthFormat, sampleCount, 640, 480,
                               false)){
            THROW("Unable to create GL Context")
        }

        GLint major = 0;
        GLint minor = 0;
        glGetIntegerv(GL_MAJOR_VERSION, &major);
        glGetIntegerv(GL_MINOR_VERSION, &minor);

        const XrVersion minApiVersionSupported = XR_MAKE_VERSION(major, minor, 0);
        LOGI("###### OpenGL ES version %d.%d is supported, minXrVersion: %lu", major, minor, graphicsRequirements.minApiVersionSupported);
        if(graphicsRequirements.minApiVersionSupported > minApiVersionSupported){
            THROW("Minimum required OpenGL ES version is %d.%d, but runtime reports %d.%d is supported")
        }

        m_contextApiMajorVersion = major;
        m_graphicsBinding.display = window.display;
        m_graphicsBinding.config = (EGLConfig) 0;
        m_graphicsBinding.context = window.context.context;

        glEnable(GL_DEBUG_OUTPUT);
        // init resources
    }

    void initialize_session() {
        LOGI("###### initialize session");
        XrSessionCreateInfo sessionCreateInfo{XR_TYPE_SESSION_CREATE_INFO};
        sessionCreateInfo.next = (XrBaseInStructure*)&m_graphicsBinding;
        sessionCreateInfo.systemId = m_systemId;
        OPENXR_CHECK(xrCreateSession(m_xrInstance, &sessionCreateInfo, &m_session), "Failed to create session.")

        uint32_t spaceCount;
        OPENXR_CHECK(xrEnumerateReferenceSpaces(m_session, 0, &spaceCount, nullptr), "Failed to enumerate reference spaces.")
        std::vector<XrReferenceSpaceType> spaces(spaceCount);
        OPENXR_CHECK(xrEnumerateReferenceSpaces(m_session, (uint32_t)spaces.size(), &spaceCount, spaces.data()),
                     "Failed to enumerate reference spaces.")

        LOGI("###### reference spaces: %d",spaceCount);
        for (XrReferenceSpaceType space: spaces) {
            LOGI("###### reference name: %s",to_string(space));
        }

        //TODO actions
        //TODO visualizedSpaces

        create_visualized_spaces();

        {
            XrReferenceSpaceCreateInfo referenceSpaceCreateInfo = GetXrReferenceSpaceCreateInfo("Local");
            OPENXR_CHECK(xrCreateReferenceSpace(m_session,&referenceSpaceCreateInfo,&m_appSpace),
                         "Failed to create reference space Local.")
        }
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
        initialize_device();
        initialize_session();
    }
}