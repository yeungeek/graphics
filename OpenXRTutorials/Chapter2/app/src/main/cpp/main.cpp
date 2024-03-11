//
// Created by jian.yang on 2024/3/6.
//

#include <DebugOutput.h>
#include <GraphicsAPI_OpenGL.h>
// XR_DOCS_TAG_END_include_GraphicsAPI_OpenGL
// XR_DOCS_TAG_BEGIN_include_GraphicsAPI_OpenGL_ES
#include <GraphicsAPI_OpenGL_ES.h>
// XR_DOCS_TAG_END_include_GraphicsAPI_OpenGL_ES
// XR_DOCS_TAG_BEGIN_include_GraphicsAPI_Vulkan
#include <GraphicsAPI_Vulkan.h>
// XR_DOCS_TAG_END_include_GraphicsAPI_Vulkan
// XR_DOCS_TAG_BEGIN_include_OpenXRDebugUtils
#include <OpenXRDebugUtils.h>
// XR_DOCS_TAG_END_include_OpenXRDebugUtils

#include <android/log.h>

#define LOGI(...) \
((void)__android_log_print( ANDROID_LOG_INFO, "native", __VA_ARGS__ ))

#define LOGW(...) \
((void)__android_log_print( ANDROID_LOG_WARN, "native", __VA_ARGS__ ))

class OpenXRTutorial {
public:
    OpenXRTutorial(GraphicsAPI_Type apiType) : m_apiType(apiType) {
        XR_TUT_LOG("###### OpenXRTutorial")
        if (!CheckGraphicsAPI_TypeIsValidForPlatform(m_apiType)) {
            std::cout << "Invalid graphics API type: " << m_apiType << std::endl;
            DEBUG_BREAK;
        }
    }

    ~OpenXRTutorial() {
        XR_TUT_LOG("###### OpenXRTutorial::~OpenXRTutorial")
    }

    void Run() {
        CreateInstance();
        CreateDebugMessenger();

        GetInstanceProperties();
        GetSystemID();

        CreateSession();
        DestroySession();

        DestroyDebugMessenger();
        DestroyInstance();

        while (m_applicationRunning) {
            PollSystemEvents();
            PollEvents();
            if (m_sessionRunning) {
                // draw
            }
        }
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
    void CreateInstance() {
        XR_TUT_LOG("###### CreateInstance")
        XrApplicationInfo AI;
        strncpy(AI.applicationName, "OpenXR Setup", XR_MAX_APPLICATION_NAME_SIZE);
        AI.applicationVersion = 1;
        strncpy(AI.engineName, "OpenXR Engine", XR_MAX_ENGINE_NAME_SIZE);
        AI.engineVersion = 1;
        AI.apiVersion = XR_CURRENT_API_VERSION;

        {
            m_instanceExtensions.push_back(XR_EXT_DEBUG_UTILS_EXTENSION_NAME);
            m_instanceExtensions.push_back(GetGraphicsAPIInstanceExtensionString(m_apiType));
        }

        // Get all the API Layers
        uint32_t apiLayerCount = 0;
        std::vector<XrApiLayerProperties> apiLayerProperties;
        OPENXR_CHECK(xrEnumerateApiLayerProperties(0, &apiLayerCount, nullptr),
                     "Failed to enumerate ApiLayerProperties.");
        apiLayerProperties.resize(apiLayerCount, {XR_TYPE_API_LAYER_PROPERTIES});
        OPENXR_CHECK(xrEnumerateApiLayerProperties(apiLayerCount, &apiLayerCount,
                                                   apiLayerProperties.data()),
                     "Failed to enumerate ApiLayerProperties.");

        for (auto &requestLayer: m_apiLayers) {
            for (auto &layerProperty: apiLayerProperties) {
                if (strcmp(requestLayer.c_str(), layerProperty.layerName) != 0) {
                    continue;
                } else {
                    m_activeAPILayers.push_back(requestLayer.c_str());
                    break;
                }
            }
        }

        // Get all the Instance Extensions
        uint32_t extensionCount = 0;
        std::vector<XrExtensionProperties> extensionProperties;
        OPENXR_CHECK(xrEnumerateInstanceExtensionProperties(nullptr, 0, &extensionCount, nullptr),
                     "Failed to enumerate InstanceExtensionProperties.");
        extensionProperties.resize(extensionCount, {XR_TYPE_EXTENSION_PROPERTIES});
        OPENXR_CHECK(xrEnumerateInstanceExtensionProperties(nullptr, extensionCount,
                                                            &extensionCount,
                                                            extensionProperties.data()),
                     "Failed to enumerate InstanceExtensionProperties.");
        for (auto &requestExtension: m_instanceExtensions) {
            bool found = false;
            for (auto &extensionProperty: extensionProperties) {
                if (strcmp(requestExtension.c_str(), extensionProperty.extensionName) != 0) {
                    continue;
                } else {
                    found = true;
                    m_activeInstanceExtensions.push_back(requestExtension.c_str());
                    break;
                }
            }

            if (!found) {
                LOGW("Failed to find extension: %s", requestExtension.c_str());
            }
        }

        XrInstanceCreateInfo instanceCI{XR_TYPE_INSTANCE_CREATE_INFO};
        instanceCI.createFlags = 0;
        instanceCI.applicationInfo = AI;
        instanceCI.enabledApiLayerCount = static_cast<uint32_t> (m_activeAPILayers.size());
        instanceCI.enabledApiLayerNames = m_activeAPILayers.data();
        instanceCI.enabledExtensionCount = static_cast<uint32_t> (m_activeInstanceExtensions.size());
        instanceCI.enabledExtensionNames = m_activeInstanceExtensions.data();
        OPENXR_CHECK(xrCreateInstance(&instanceCI, &m_xrInstance), "Failed to create instance.")
    }

    void DestroyInstance() {
        XR_TUT_LOG("###### DestroyInstance")
        OPENXR_CHECK(xrDestroyInstance(m_xrInstance), "Failed to destroy instance.")
    }

    void CreateDebugMessenger() {
        XR_TUT_LOG("###### CreateDebugMessenger")
        if (IsStringInVector(m_activeInstanceExtensions, XR_EXT_DEBUG_UTILS_EXTENSION_NAME)) {
            m_debugUtilsMessenger = CreateOpenXRDebugUtilsMessenger(m_xrInstance);
        }
    }

    void DestroyDebugMessenger() {
        XR_TUT_LOG("###### DestroyDebugMessenger")
        if (m_debugUtilsMessenger != XR_NULL_HANDLE) {
            DestroyOpenXRDebugUtilsMessenger(m_xrInstance, m_debugUtilsMessenger);
        }
    }

    void GetInstanceProperties() {
        XrInstanceProperties instanceProperties{XR_TYPE_INSTANCE_PROPERTIES};
        OPENXR_CHECK(xrGetInstanceProperties(m_xrInstance, &instanceProperties),
                     "Failed to get instance properties.")
        XR_TUT_LOG("###### GetInstanceProperties: " << instanceProperties.runtimeName << "-"
                                                    << XR_VERSION_MAJOR(
                                                            instanceProperties.runtimeVersion)
                                                    << "."
                                                    << XR_VERSION_MINOR(
                                                            instanceProperties.runtimeVersion)
                                                    << "."
                                                    << XR_VERSION_PATCH(
                                                            instanceProperties.runtimeVersion));
    }

    void GetSystemID() {
        XR_TUT_LOG("###### GetSystemID")
        XrSystemGetInfo systemGI{XR_TYPE_SYSTEM_GET_INFO};
        systemGI.formFactor = m_formFactor;
        OPENXR_CHECK(xrGetSystem(m_xrInstance, &systemGI, &m_systemID), "Failed to get system id.")
        OPENXR_CHECK(xrGetSystemProperties(m_xrInstance, m_systemID, &m_systemProperties),
                     "Failed to get system properties.")
    }

    void CreateSession() {
        XR_TUT_LOG("###### CreateSession")
        XrSessionCreateInfo sessionCI{XR_TYPE_SESSION_CREATE_INFO};
        m_graphicsAPI = std::make_unique<GraphicsAPI_OpenGL_ES>(m_xrInstance, m_systemID);
        sessionCI.next = m_graphicsAPI->GetGraphicsBinding();
        sessionCI.createFlags = 0;
        sessionCI.systemId = m_systemID;
        OPENXR_CHECK(xrCreateSession(m_xrInstance, &sessionCI, &m_session),
                     "Failed to create session.")
    }

    void DestroySession() {
        XR_TUT_LOG("###### DestroySession")
        OPENXR_CHECK(xrDestroySession(m_session), "Failed to destroy session.")
    }

    void PollSystemEvents() {
        XR_TUT_LOG("###### PollSystemEvents")

        if (androidApp->destroyRequested != 0) {
            m_applicationRunning = false;
            return;
        }

        while (true) {
            XR_TUT_LOG("###### while")
            struct android_poll_source *source = nullptr;
            int events = 0;

            // The timeout depends on whether the application is active.
            const int timeoutMilliseconds = (!androidAppState.resumed && !m_sessionRunning &&
                                             androidApp->destroyRequested == 0) ? -1 : 0;
            LOGI("###### timeoutMilliseconds: %d", timeoutMilliseconds);
            if (ALooper_pollAll(timeoutMilliseconds, nullptr, &events, (void **) &source) >= 0) {
                XR_TUT_LOG("###### ALooper_pollAll")
                if (source != nullptr) {
                    source->process(androidApp, source);
                }
            } else {
                XR_TUT_LOG("###### break")
                break;
            }
        }
    }

    void PollEvents() {
        // Poll OpenXR for a new event
        XrEventDataBuffer eventData{XR_TYPE_EVENT_DATA_BUFFER};
        auto XrPollEvent = [&]() -> bool {
            eventData = {XR_TYPE_EVENT_DATA_BUFFER};
            return xrPollEvent(m_xrInstance, &eventData) == XR_SUCCESS;
        };

        while (XrPollEvent()) {
            switch (eventData.type) {
                case XR_TYPE_EVENT_DATA_EVENTS_LOST: {
                    XrEventDataEventsLost *eventsLost = reinterpret_cast<XrEventDataEventsLost *>(&eventData);
                    XR_TUT_LOG("###### Events lost: " << eventsLost->lostEventCount)
                    break;
                }
                case XR_TYPE_EVENT_DATA_INSTANCE_LOSS_PENDING: {
                    XrEventDataInstanceLossPending *instanceLossPending =
                            reinterpret_cast<XrEventDataInstanceLossPending *>(&eventData);
                    XR_TUT_LOG("###### Instance loss pending at " << instanceLossPending->lossTime)
                    m_sessionRunning = false;
                    m_applicationRunning = false;
                    break;
                }
                case XR_TYPE_EVENT_DATA_INTERACTION_PROFILE_CHANGED: {
                    XrEventDataInteractionProfileChanged *interactionProfileChanged =
                            reinterpret_cast<XrEventDataInteractionProfileChanged *>(&eventData);
                    XR_TUT_LOG("###### Interaction profile changed at "
                                       << interactionProfileChanged->session)
                    if (interactionProfileChanged->session != m_session) {
                        XR_TUT_LOG("XrEventDataInteractionProfileChanged for unknown state")
                        break;
                    }
                    break;
                }
                case XR_TYPE_EVENT_DATA_REFERENCE_SPACE_CHANGE_PENDING: {
                    XrEventDataReferenceSpaceChangePending *referenceSpaceChangePending =
                            reinterpret_cast<XrEventDataReferenceSpaceChangePending *>(&eventData);
                    XR_TUT_LOG("###### Reference space change pending for session: "
                                       << referenceSpaceChangePending->session)
                    if (referenceSpaceChangePending->session != m_session) {
                        XR_TUT_LOG("XrEventDataReferenceSpaceChangePending for unknown state")
                        break;
                    }
                    break;
                }
                case XR_TYPE_EVENT_DATA_SESSION_STATE_CHANGED: {
                    XrEventDataSessionStateChanged *sessionStateChanged =
                            reinterpret_cast<XrEventDataSessionStateChanged *>(&eventData);
                    XR_TUT_LOG("###### Session state changed to " << sessionStateChanged->state)
                    if (sessionStateChanged->session != m_session) {
                        XR_TUT_LOG("XrEventDataSessionStateChanged for unknown state")
                        break;
                    }

                    if (sessionStateChanged->state == XR_SESSION_STATE_READY) {
                        // ready
                        m_sessionRunning = true;
                        XrSessionBeginInfo sessionBeginInfo{XR_TYPE_SESSION_BEGIN_INFO};
                        sessionBeginInfo.primaryViewConfigurationType = XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO;
                        OPENXR_CHECK(xrBeginSession(m_session, &sessionBeginInfo),
                                     "Failed to begin session.")
                    }

                    if (sessionStateChanged->state == XR_SESSION_STATE_STOPPING) {
                        OPENXR_CHECK(xrEndSession(m_session), "Failed to end session.")
                        m_sessionRunning = false;
                    }

                    if (sessionStateChanged->state == XR_SESSION_STATE_EXITING) {
                        m_sessionRunning = false;
                        m_applicationRunning = false;
                    }

                    if (sessionStateChanged->state == XR_SESSION_STATE_LOSS_PENDING) {
                        m_sessionRunning = false;
                        m_applicationRunning = false;
                    }

                    m_sessionState = sessionStateChanged->state;
                    break;
                }
                default:
                    break;
            }
        }
    }

private:
    bool m_applicationRunning = true;
    bool m_sessionRunning = false;

    XrInstance m_xrInstance = XR_NULL_HANDLE;
    std::vector<const char *> m_activeAPILayers = {};
    std::vector<const char *> m_activeInstanceExtensions = {};
    std::vector<std::string> m_apiLayers = {};
    std::vector<std::string> m_instanceExtensions = {};

    XrDebugUtilsMessengerEXT m_debugUtilsMessenger = {};

    XrFormFactor m_formFactor = XR_FORM_FACTOR_HEAD_MOUNTED_DISPLAY;
    XrSystemId m_systemID = {};
    XrSystemProperties m_systemProperties = {XR_TYPE_SYSTEM_PROPERTIES};

    GraphicsAPI_Type m_apiType = UNKNOWN;

    std::unique_ptr<GraphicsAPI> m_graphicsAPI = nullptr;

    XrSession m_session = XR_NULL_HANDLE;
    XrSessionState m_sessionState = XR_SESSION_STATE_UNKNOWN;
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

    XrResult m_procAddr = xrGetInstanceProcAddr(XR_NULL_HANDLE, "xrInitializeLoaderKHR",
                                                (PFN_xrVoidFunction *) &xrInitializeLoaderKHR);
    LOGI("###### xrGetInstanceProcAddr: %d", m_procAddr);
    OPENXR_CHECK(m_procAddr, "Failed to xrGetInstanceProcAddr")

    // Fill out an XrLoaderInitInfoAndroidKHR structure and initialize the loader for Android.
    if (!xrInitializeLoaderKHR) {
        return;
    }

    XrLoaderInitInfoAndroidKHR loaderInitializeInfoAndroid{XR_TYPE_LOADER_INIT_INFO_ANDROID_KHR};
    loaderInitializeInfoAndroid.applicationVM = app->activity->vm;
    loaderInitializeInfoAndroid.applicationContext = app->activity->clazz;

    XrResult m_xrLoaderKHR = xrInitializeLoaderKHR(
            (XrLoaderInitInfoBaseHeaderKHR *) &loaderInitializeInfoAndroid);
    LOGI("###### xrInitializeLoaderKHR: %d", m_xrLoaderKHR);
    OPENXR_CHECK(m_xrLoaderKHR, "Failed to initialize Loader for Android");

    app->userData = &OpenXRTutorial::androidAppState;
    app->onAppCmd = OpenXRTutorial::AndroidAppHandleCmd;

    OpenXRTutorial::androidApp = app;

    OpenXRTutorialMain(XR_TUTORIAL_GRAPHICS_API);
}