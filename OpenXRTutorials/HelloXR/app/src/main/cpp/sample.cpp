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
#include <list>
#include <map>
#include <thread>

#include "../openxr/openxr_platform.h"
#include "common/openxr_helper.h"
#include "common/gfxwrapper_opengl.h"
#include "common/common.h"
#include "common/geometry.h"
#include "common/xr_linear.h"

#define LOG_TAG "sample"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGW(...) __android_log_print(ANDROID_LOG_WARN, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)
#define strcpy_s(dest, source) strncpy((dest), (source), sizeof(dest))

namespace Sample {

    namespace Side {
        const int LEFT = 0;
        const int RIGHT = 1;
        const int COUNT = 2;
    }  // namespace Side

    static const char *VertexShaderGlsl = R"_(#version 320 es

    in vec3 VertexPos;
    in vec3 VertexColor;

    out vec3 PSVertexColor;

    uniform mat4 ModelViewProjection;

    void main() {
       gl_Position = ModelViewProjection * vec4(VertexPos, 1.0);
       PSVertexColor = VertexColor;
    }
    )_";

    // The version statement has come on first line.
    static const char *FragmentShaderGlsl = R"_(#version 320 es

    in lowp vec3 PSVertexColor;
    out lowp vec4 FragColor;

    void main() {
       FragColor = vec4(PSVertexColor, 1);
    }
    )_";


    struct AndroidAppState {
        ANativeWindow *NativeWindow = nullptr;
        bool Resumed = false;
    };

    struct Swapchain {
        XrSwapchain handle;
        int32_t width;
        int32_t height;
    };

    struct Cube {
        XrPosef Pose;
        XrVector3f Scale;
    };

    struct InputState{
        XrActionSet actionSet{XR_NULL_HANDLE};
        XrAction grabAction{XR_NULL_HANDLE};
        XrAction poseAction{XR_NULL_HANDLE};
        XrAction vibrateAction{XR_NULL_HANDLE};
        XrAction quitAction{XR_NULL_HANDLE};

        std::array<XrPath,Side::COUNT> handSubactionPath;
        std::array<XrSpace,Side::COUNT> handSpace;
        std::array<float,Side::COUNT> handScale = {{1.0f,1.0f}};
        std::array<XrBool32,Side::COUNT> handActive;
    };

    // properties
    void *applicationVM;
    void *applicationActivity;

    // openxr
    XrInstanceCreateInfoAndroidKHR instanceCreateInfoAndroid;
    XrInstance m_xrInstance{XR_NULL_HANDLE};
    XrSession m_session{XR_NULL_HANDLE};
    XrSystemId m_systemId{XR_NULL_SYSTEM_ID};
    XrSpace m_appSpace{XR_NULL_HANDLE};

    // config
    XrViewConfigurationType viewConfigType{XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO};
    XrEnvironmentBlendMode environmentBlendMode{XR_ENVIRONMENT_BLEND_MODE_OPAQUE};

    XrGraphicsBindingOpenGLESAndroidKHR m_graphicsBinding{
            XR_TYPE_GRAPHICS_BINDING_OPENGL_ES_ANDROID_KHR};

    XrEventDataBuffer m_eventDataBuffer;

    std::vector<XrSpace> m_visualizedSpaces;
    std::vector<XrViewConfigurationView> m_configViews;
    std::vector<Swapchain> m_swapchains;

    std::map<XrSwapchain, std::vector<XrSwapchainImageBaseHeader *>> m_swapchainImages;
    std::list<std::vector<XrSwapchainImageOpenGLESKHR>> m_swapchainImageBuffers;
    std::vector<XrView> m_views;
    int64_t m_colorSwapchainFormat{-1};

    // Map color buffer to associated depth buffer. This map is populated on demand.
    std::map<uint32_t, uint32_t> m_colorToDepthMap;
    std::array<float, 4> m_clearColor = {0.184313729f, 0.309803933f, 0.309803933f, 1.0f};
//    std::array<float, 4> m_clearColor = {0.0f, 0.0f, 0.0f, 0.0f};

    ksGpuWindow window{};

    GLint m_contextApiMajorVersion{0};
    GLuint m_swapchainFramebuffer{0};
    GLuint m_program{0};
    GLint m_modelViewProjectionUniformLocation{0};
    GLint m_vertexAttribCoords{0};
    GLint m_vertexAttribColor{0};
    GLuint m_vao{0};
    GLuint m_cubeVertexBuffer{0};
    GLuint m_cubeIndexBuffer{0};

    XrSessionState m_sessionState{XR_SESSION_STATE_UNKNOWN};
    bool m_sessionRunning{false};
    bool requestRestart = false;
    bool exitRenderLoop = false;

    InputState m_input;

    /**
     * Process the next main command.
     */
    static void app_handle_cmd(struct android_app *app, int32_t cmd) {
        AndroidAppState *appState = (AndroidAppState *) app->userData;

        switch (cmd) {
            // There is no APP_CMD_CREATE. The ANativeActivity creates the
            // application thread from onCreate(). The application thread
            // then calls android_main().
            case APP_CMD_START: {
                LOGI("    APP_CMD_START");
                LOGI("onStart()");
                break;
            }
            case APP_CMD_RESUME: {
                LOGI("onResume()");
                LOGI("    APP_CMD_RESUME");
                appState->Resumed = true;
                break;
            }
            case APP_CMD_PAUSE: {
                LOGI("onPause()");
                LOGI("    APP_CMD_PAUSE");
                appState->Resumed = false;
                break;
            }
            case APP_CMD_STOP: {
                LOGI("onStop()");
                LOGI("    APP_CMD_STOP");
                break;
            }
            case APP_CMD_DESTROY: {
                LOGI("onDestroy()");
                LOGI("    APP_CMD_DESTROY");
                appState->NativeWindow = NULL;
                break;
            }
            case APP_CMD_INIT_WINDOW: {
                LOGI("surfaceCreated()");
                LOGI("    APP_CMD_INIT_WINDOW");
                appState->NativeWindow = app->window;
                break;
            }
            case APP_CMD_TERM_WINDOW: {
                LOGI("surfaceDestroyed()");
                LOGI("    APP_CMD_TERM_WINDOW");
                appState->NativeWindow = NULL;
                break;
            }
        }
    }

    void CheckShader(GLuint shader) {
        GLint r = 0;
        glGetShaderiv(shader, GL_COMPILE_STATUS, &r);
        if (r == GL_FALSE) {
            GLchar msg[4096] = {};
            GLsizei length;
            glGetShaderInfoLog(shader, sizeof(msg), &length, msg);
            THROW(Fmt("Compile shader failed: %s", msg));
        }
    }

    void CheckProgram(GLuint prog) {
        GLint r = 0;
        glGetProgramiv(prog, GL_LINK_STATUS, &r);
        if (r == GL_FALSE) {
            GLchar msg[4096] = {};
            GLsizei length;
            glGetProgramInfoLog(prog, sizeof(msg), &length, msg);
            THROW(Fmt("Link program failed: %s", msg));
        }
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

            XrPosef Translation(const XrVector3f &translation) {
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

    inline XrReferenceSpaceCreateInfo
    GetXrReferenceSpaceCreateInfo(const std::string &referenceSpaceTypeStr) {
        XrReferenceSpaceCreateInfo referenceSpaceCreateInfo{XR_TYPE_REFERENCE_SPACE_CREATE_INFO};
        referenceSpaceCreateInfo.poseInReferenceSpace = Math::Pose::Identity();
        if (EqualsIgnoreCase(referenceSpaceTypeStr, "View")) {
            referenceSpaceCreateInfo.referenceSpaceType = XR_REFERENCE_SPACE_TYPE_VIEW;
        } else if (EqualsIgnoreCase(referenceSpaceTypeStr, "ViewFront")) {
            // Render head-locked 2m in front of device.
            referenceSpaceCreateInfo.poseInReferenceSpace = Math::Pose::Translation(
                    {0.f, 0.f, -2.f}),
                    referenceSpaceCreateInfo.referenceSpaceType = XR_REFERENCE_SPACE_TYPE_VIEW;
        } else if (EqualsIgnoreCase(referenceSpaceTypeStr, "Local")) {
            referenceSpaceCreateInfo.referenceSpaceType = XR_REFERENCE_SPACE_TYPE_LOCAL;
        } else if (EqualsIgnoreCase(referenceSpaceTypeStr, "Stage")) {
            referenceSpaceCreateInfo.referenceSpaceType = XR_REFERENCE_SPACE_TYPE_STAGE;
        } else if (EqualsIgnoreCase(referenceSpaceTypeStr, "StageLeft")) {
            referenceSpaceCreateInfo.poseInReferenceSpace = Math::Pose::RotateCCWAboutYAxis(0.f,
                                                                                            {-2.f,
                                                                                             0.f,
                                                                                             -2.f});
            referenceSpaceCreateInfo.referenceSpaceType = XR_REFERENCE_SPACE_TYPE_STAGE;
        } else if (EqualsIgnoreCase(referenceSpaceTypeStr, "StageRight")) {
            referenceSpaceCreateInfo.poseInReferenceSpace = Math::Pose::RotateCCWAboutYAxis(0.f,
                                                                                            {2.f,
                                                                                             0.f,
                                                                                             -2.f});
            referenceSpaceCreateInfo.referenceSpaceType = XR_REFERENCE_SPACE_TYPE_STAGE;
        } else if (EqualsIgnoreCase(referenceSpaceTypeStr, "StageLeftRotated")) {
            referenceSpaceCreateInfo.poseInReferenceSpace = Math::Pose::RotateCCWAboutYAxis(
                    3.14f / 3.f, {-2.f, 0.5f, -2.f});
            referenceSpaceCreateInfo.referenceSpaceType = XR_REFERENCE_SPACE_TYPE_STAGE;
        } else if (EqualsIgnoreCase(referenceSpaceTypeStr, "StageRightRotated")) {
            referenceSpaceCreateInfo.poseInReferenceSpace = Math::Pose::RotateCCWAboutYAxis(
                    -3.14f / 3.f, {2.f, 0.5f, -2.f});
            referenceSpaceCreateInfo.referenceSpaceType = XR_REFERENCE_SPACE_TYPE_STAGE;
        } else {
            throw std::invalid_argument(
                    Fmt("Unknown reference space type '%s'", referenceSpaceTypeStr.c_str()));
        }
        return referenceSpaceCreateInfo;
    }

    int64_t SelectColorSwapchainFormat(const std::vector<int64_t> &runtimeFormats) {
        // List of supported color swapchain formats.
        std::vector<int64_t> supportedColorSwapchainFormats{GL_RGBA8, GL_RGBA8_SNORM};

        // In OpenGLES 3.0+, the R, G, and B values after blending are converted into the non-linear
        // sRGB automatically.
        if (m_contextApiMajorVersion >= 3) {
            supportedColorSwapchainFormats.push_back(GL_SRGB8_ALPHA8);
        }

        auto swapchainFormatIt = std::find_first_of(runtimeFormats.begin(), runtimeFormats.end(),
                                                    supportedColorSwapchainFormats.begin(),
                                                    supportedColorSwapchainFormats.end());
        if (swapchainFormatIt == runtimeFormats.end()) {
            THROW("No runtime swapchain format supported for color swapchain");
        }

        return *swapchainFormatIt;
    }

    uint32_t GetDepthTexture(uint32_t colorTexture) {
        // If a depth-stencil view has already been created for this back-buffer, use it.
        auto depthBufferIt = m_colorToDepthMap.find(colorTexture);
        if (depthBufferIt != m_colorToDepthMap.end()) {
            return depthBufferIt->second;
        }

        // This back-buffer has no corresponding depth-stencil texture, so create one with matching dimensions.

        GLint width;
        GLint height;
        glBindTexture(GL_TEXTURE_2D, colorTexture);
        glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &width);
        glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_HEIGHT, &height);

        uint32_t depthTexture;
        glGenTextures(1, &depthTexture);
        glBindTexture(GL_TEXTURE_2D, depthTexture);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, width, height, 0, GL_DEPTH_COMPONENT,
                     GL_UNSIGNED_INT, nullptr);

        m_colorToDepthMap.insert(std::make_pair(colorTexture, depthTexture));

        return depthTexture;
    }

    bool IsSessionRunning() { return m_sessionRunning; }

    void HandleSessionStateChangedEvent(const XrEventDataSessionStateChanged &stateChangedEvent,
                                        bool *exitRenderLoop,
                                        bool *requestRestart) {
        const XrSessionState oldState = m_sessionState;
        m_sessionState = stateChangedEvent.state;

        LOGI("XrEventDataSessionStateChanged: state %s->%s session=%lld time=%ld",
             to_string(oldState),
             to_string(m_sessionState), stateChangedEvent.session, stateChangedEvent.time);

        if ((stateChangedEvent.session != XR_NULL_HANDLE) &&
            (stateChangedEvent.session != m_session)) {
            LOGW("###### XrEventDataSessionStateChanged for unknown session");
            return;
        }

        switch (m_sessionState) {
            case XR_SESSION_STATE_READY: {
                XrSessionBeginInfo sessionBeginInfo{XR_TYPE_SESSION_BEGIN_INFO};
                sessionBeginInfo.primaryViewConfigurationType = viewConfigType;
                xrBeginSession(m_session, &sessionBeginInfo);
                m_sessionRunning = true;

                //TODO for test
//                std::this_thread::sleep_for(std::chrono::seconds (50));
//                LOGW("###### start xrRequestExitSession");
//                xrRequestExitSession(m_session);
                break;
            }
            case XR_SESSION_STATE_STOPPING: {
                m_sessionRunning = false;
                xrEndSession(m_session);
                break;
            }
            case XR_SESSION_STATE_EXITING: {
                *exitRenderLoop = true;
                *requestRestart = false;
                break;
            }
            case XR_SESSION_STATE_LOSS_PENDING: {
                *exitRenderLoop = true;
                *requestRestart = true;
                break;
            }
            default:
                break;
        }
    }

    const XrEventDataBaseHeader *TryReadNextEvent() {
        XrEventDataBaseHeader *baseHeader = reinterpret_cast<XrEventDataBaseHeader *>(&m_eventDataBuffer);
        *baseHeader = {XR_TYPE_EVENT_DATA_BUFFER};
        const XrResult xr = xrPollEvent(m_xrInstance, &m_eventDataBuffer);
        if (xr == XR_SUCCESS) {
            if (baseHeader->type == XR_TYPE_EVENT_DATA_EVENTS_LOST) {
                const XrEventDataEventsLost *const eventsLost = reinterpret_cast<const XrEventDataEventsLost *>(baseHeader);
                LOGI("###### %d events lost", eventsLost->lostEventCount);
            }

            return baseHeader;
        }

        if (xr == XR_EVENT_UNAVAILABLE) {
            return nullptr;
        }

        THROW_XR(xr, "xrPollEvent")
    }

    void PollEvents(bool *exitRenderLoop, bool *requestRestart) {
        *exitRenderLoop = *requestRestart = false;

        while (const XrEventDataBaseHeader *event = TryReadNextEvent()) {
            switch (event->type) {
                case XR_TYPE_EVENT_DATA_INSTANCE_LOSS_PENDING: {
                    const auto &instanceLost = *reinterpret_cast<const XrEventDataInstanceLossPending *>(event);
                    LOGI("###### XrEventDataInstanceLossPending by %ld", instanceLost.lossTime);
                    *exitRenderLoop = true;
                    *requestRestart = true;
                    return;
                }
                case XR_TYPE_EVENT_DATA_SESSION_STATE_CHANGED: {
                    auto sessionStateChanged = *reinterpret_cast<const XrEventDataSessionStateChanged *>(event);
                    HandleSessionStateChangedEvent(sessionStateChanged, exitRenderLoop,
                                                   requestRestart);
                    break;
                }
                case XR_TYPE_EVENT_DATA_INTERACTION_PROFILE_CHANGED: {
                    break;
                }
                case XR_TYPE_EVENT_DATA_REFERENCE_SPACE_CHANGE_PENDING: {
                    break;
                }
                default: {
                    LOGI("###### Ignoring event type %d", event->type);
                    break;
                }
            }
        }
    }


    std::vector<XrSwapchainImageBaseHeader *> AllocateSwapchainImageStructs(
            uint32_t capacity, const XrSwapchainCreateInfo & /*swapchainCreateInfo*/) {
        // Allocate and initialize the buffer of image structs (must be sequential in memory for xrEnumerateSwapchainImages).
        // Return back an array of pointers to each swapchain image struct so the consumer doesn't need to know the type/size.
        std::vector<XrSwapchainImageOpenGLESKHR> swapchainImageBuffer(capacity,
                                                                      {XR_TYPE_SWAPCHAIN_IMAGE_OPENGL_ES_KHR});
        std::vector<XrSwapchainImageBaseHeader *> swapchainImageBase;
        for (XrSwapchainImageOpenGLESKHR &image: swapchainImageBuffer) {
            swapchainImageBase.push_back(reinterpret_cast<XrSwapchainImageBaseHeader *>(&image));
        }

        // Keep the buffer alive by moving it into the list of buffers.
        m_swapchainImageBuffers.push_back(std::move(swapchainImageBuffer));

        return swapchainImageBase;
    }

    /**
     * logic
     */
    void create_android_instance() {
        instanceCreateInfoAndroid = {XR_TYPE_INSTANCE_CREATE_INFO_ANDROID_KHR};
        instanceCreateInfoAndroid.applicationVM = applicationVM;
        instanceCreateInfoAndroid.applicationActivity = applicationActivity;
    }

    void create_openxr_instance() {
        LOGI("###### create openxr instance");
        std::vector<const char *> extensions;
        const std::vector<std::string> platformExtensions = {
                XR_KHR_ANDROID_CREATE_INSTANCE_EXTENSION_NAME};
        std::transform(platformExtensions.begin(), platformExtensions.end(),
                       std::back_inserter(extensions),
                       [](const std::string &ext) { return ext.c_str(); });

        const std::vector<std::string> graphicsExtensions = {
                XR_KHR_OPENGL_ES_ENABLE_EXTENSION_NAME};
        std::transform(graphicsExtensions.begin(), graphicsExtensions.end(),
                       std::back_inserter(extensions),
                       [](const std::string &ext) { return ext.c_str(); });

        //TODO hand tracking

        XrInstanceCreateInfo createInfo{XR_TYPE_INSTANCE_CREATE_INFO};
        createInfo.next = (XrBaseInStructure *) &instanceCreateInfoAndroid;
        createInfo.enabledExtensionCount = (uint32_t) extensions.size();
        createInfo.enabledExtensionNames = extensions.data();

        strcpy(createInfo.applicationInfo.applicationName, "OpenXR Sample");
        createInfo.applicationInfo.apiVersion = XR_CURRENT_API_VERSION;
        OPENXR_CHECK(xrCreateInstance(&createInfo, &m_xrInstance),
                     "Failed to create OpenXR instance.")
    }

    void create_visualized_spaces() {
        LOGI("###### create visualized spaces");
        std::string visualizedSpaces[] = {"ViewFront", "Local", "Stage", "StageLeft", "StageRight",
                                          "StageLeftRotated",
                                          "StageRightRotated"};
        for (const auto &visualizedSpace: visualizedSpaces) {
            XrReferenceSpaceCreateInfo referenceSpaceCreateInfo = GetXrReferenceSpaceCreateInfo(
                    visualizedSpace);
            XrSpace space;
            XrResult res = xrCreateReferenceSpace(m_session, &referenceSpaceCreateInfo, &space);

            if (XR_SUCCEEDED(res)) {
                m_visualizedSpaces.push_back(space);
            } else {
                LOGE("###### Failed to create reference space %s with error %d",
                     visualizedSpace.c_str(), res);
            }
        }
    }

    void create_swapchains() {
        XrSystemProperties systemProperties{XR_TYPE_SYSTEM_PROPERTIES};
        OPENXR_CHECK(xrGetSystemProperties(m_xrInstance, m_systemId, &systemProperties),
                     "Failed to get system properties.")
        LOGI("###### Using system %d with name %s and vendorId %d", m_systemId,
             systemProperties.systemName, systemProperties.vendorId);

        // Query and cache view configuration views.
        uint32_t viewCount;
        OPENXR_CHECK(xrEnumerateViewConfigurationViews(m_xrInstance, m_systemId, viewConfigType, 0,
                                                       &viewCount,
                                                       nullptr),
                     "Failed to enumerate configuration views")
        m_configViews.resize(viewCount, {XR_TYPE_VIEW_CONFIGURATION_VIEW});
        OPENXR_CHECK(xrEnumerateViewConfigurationViews(m_xrInstance, m_systemId, viewConfigType,
                                                       viewCount, &viewCount,
                                                       m_configViews.data()),
                     "Failed to enumerate configuration views")
        // Create and cache view buffer for xrLocateViews later.
        m_views.resize(viewCount, {XR_TYPE_VIEW});

        if (viewCount > 0) {
            uint32_t swapchainFormatCount;
            OPENXR_CHECK(xrEnumerateSwapchainFormats(m_session, 0, &swapchainFormatCount, nullptr),
                         "Failed to get swapchain format count.")
            std::vector<int64_t> swapchainFormats(swapchainFormatCount);
            OPENXR_CHECK(xrEnumerateSwapchainFormats(m_session, (uint32_t) swapchainFormats.size(),
                                                     &swapchainFormatCount,
                                                     swapchainFormats.data()),
                         "Failed to get swapchain formats.")


            m_colorSwapchainFormat = SelectColorSwapchainFormat(swapchainFormats);

            // Print swapchain formats and the selected one.
            {
                std::string swapchainFormatsString;
                for (int64_t format: swapchainFormats) {
                    const bool selected = format == m_colorSwapchainFormat;
                    swapchainFormatsString += " ";
                    if (selected) {
                        swapchainFormatsString += "[";
                    }
                    swapchainFormatsString += std::to_string(format);
                    if (selected) {
                        swapchainFormatsString += "]";
                    }
                }
                LOGI("###### Swapchain formats: %s, selected format: %d",
                     swapchainFormatsString.c_str(),
                     m_colorSwapchainFormat);
            }

            // Create a swapchain for each view.
            for (uint32_t i = 0; i < viewCount; i++) {
                const XrViewConfigurationView &vp = m_configViews[i];
                LOGI("Creating swapchain for view %d with dimensions Width=%d Height=%d SampleCount=%d",
                     i,
                     vp.recommendedImageRectWidth, vp.recommendedImageRectHeight,
                     vp.recommendedSwapchainSampleCount);

                // Create the swapchain.
                XrSwapchainCreateInfo swapchainCreateInfo{XR_TYPE_SWAPCHAIN_CREATE_INFO};
                swapchainCreateInfo.arraySize = 1;
                swapchainCreateInfo.format = m_colorSwapchainFormat;
                swapchainCreateInfo.width = vp.recommendedImageRectWidth;
                swapchainCreateInfo.height = vp.recommendedImageRectHeight;
                swapchainCreateInfo.mipCount = 1;
                swapchainCreateInfo.faceCount = 1;
                swapchainCreateInfo.sampleCount = vp.recommendedSwapchainSampleCount;
                swapchainCreateInfo.usageFlags =
                        XR_SWAPCHAIN_USAGE_SAMPLED_BIT | XR_SWAPCHAIN_USAGE_COLOR_ATTACHMENT_BIT;
                Swapchain swapchain;
                swapchain.width = swapchainCreateInfo.width;
                swapchain.height = swapchainCreateInfo.height;
                OPENXR_CHECK(xrCreateSwapchain(m_session, &swapchainCreateInfo, &swapchain.handle),
                             "Failed to create swapchain.")

                m_swapchains.push_back(swapchain);

                uint32_t imageCount;
                OPENXR_CHECK(xrEnumerateSwapchainImages(swapchain.handle, 0, &imageCount, nullptr),
                             "Failed to get swapchain image count.")
                std::vector<XrSwapchainImageBaseHeader *> swapchainImages = AllocateSwapchainImageStructs(
                        imageCount, swapchainCreateInfo);
                OPENXR_CHECK(xrEnumerateSwapchainImages(swapchain.handle, imageCount, &imageCount,
                                                        swapchainImages[0]),
                             "Failed to enumerate swapchain images.")

                m_swapchainImages.insert(
                        std::make_pair(swapchain.handle, std::move(swapchainImages)));
            }
        }
    }

    void initialize_system() {
        XrSystemGetInfo systemInfo{XR_TYPE_SYSTEM_GET_INFO};
        systemInfo.formFactor = XR_FORM_FACTOR_HEAD_MOUNTED_DISPLAY;
        OPENXR_CHECK(xrGetSystem(m_xrInstance, &systemInfo, &m_systemId),
                     "Failed to get system info.")
        LOGI("###### Using system %d for form factor %s", m_systemId,
             to_string(systemInfo.formFactor));
    }

    void initialize_resources() {
        glGenFramebuffers(1, &m_swapchainFramebuffer);

        GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
        glShaderSource(vertexShader, 1, &VertexShaderGlsl, nullptr);
        glCompileShader(vertexShader);
        CheckShader(vertexShader);

        GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
        glShaderSource(fragmentShader, 1, &FragmentShaderGlsl, nullptr);
        glCompileShader(fragmentShader);
        CheckShader(fragmentShader);

        m_program = glCreateProgram();
        glAttachShader(m_program, vertexShader);
        glAttachShader(m_program, fragmentShader);
        glLinkProgram(m_program);
        CheckProgram(m_program);

        glDeleteShader(vertexShader);
        glDeleteShader(fragmentShader);

        m_modelViewProjectionUniformLocation = glGetUniformLocation(m_program,
                                                                    "ModelViewProjection");

        m_vertexAttribCoords = glGetAttribLocation(m_program, "VertexPos");
        m_vertexAttribColor = glGetAttribLocation(m_program, "VertexColor");

        glGenBuffers(1, &m_cubeVertexBuffer);
        glBindBuffer(GL_ARRAY_BUFFER, m_cubeVertexBuffer);
        glBufferData(GL_ARRAY_BUFFER, sizeof(Geometry::c_cubeVertices), Geometry::c_cubeVertices,
                     GL_STATIC_DRAW);

        glGenBuffers(1, &m_cubeIndexBuffer);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_cubeIndexBuffer);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(Geometry::c_cubeIndices),
                     Geometry::c_cubeIndices, GL_STATIC_DRAW);

        glGenVertexArrays(1, &m_vao);
        glBindVertexArray(m_vao);
        glEnableVertexAttribArray(m_vertexAttribCoords);
        glEnableVertexAttribArray(m_vertexAttribColor);
        glBindBuffer(GL_ARRAY_BUFFER, m_cubeVertexBuffer);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_cubeIndexBuffer);
        glVertexAttribPointer(m_vertexAttribCoords, 3, GL_FLOAT, GL_FALSE, sizeof(Geometry::Vertex),
                              nullptr);
        glVertexAttribPointer(m_vertexAttribColor, 3, GL_FLOAT, GL_FALSE, sizeof(Geometry::Vertex),
                              reinterpret_cast<const void *>(sizeof(XrVector3f)));
    }

    void initialize_device() {
        // Extension function must be loaded by name
        LOGI("###### initialize device");
        PFN_xrGetOpenGLESGraphicsRequirementsKHR pfnGetOpenGLESGraphicsRequirementsKHR = nullptr;
        OPENXR_CHECK(xrGetInstanceProcAddr(m_xrInstance, "xrGetOpenGLESGraphicsRequirementsKHR",
                                           reinterpret_cast<PFN_xrVoidFunction *>(&pfnGetOpenGLESGraphicsRequirementsKHR)),
                     "Failed to get InstanceProcAddr for xrGetOpenGLESGraphicsRequirementsKHR.")

        XrGraphicsRequirementsOpenGLESKHR graphicsRequirements{
                XR_TYPE_GRAPHICS_REQUIREMENTS_OPENGL_ES_KHR};
        OPENXR_CHECK(pfnGetOpenGLESGraphicsRequirementsKHR(m_xrInstance, m_systemId,
                                                           &graphicsRequirements),
                     "Failed to get OpenGL ES graphics requirements.")

        // Initialize gl extension
        ksDriverInstance driverInstance{};
        ksGpuQueueInfo queueInfo{};
        ksGpuSurfaceColorFormat colorFormat{KS_GPU_SURFACE_COLOR_FORMAT_B8G8R8A8};
        ksGpuSurfaceDepthFormat depthFormat{KS_GPU_SURFACE_DEPTH_FORMAT_D24};
        ksGpuSampleCount sampleCount{KS_GPU_SAMPLE_COUNT_1};
        if (!ksGpuWindow_Create(&window, &driverInstance, &queueInfo, 0, colorFormat, depthFormat,
                                sampleCount, 640, 480,
                                false)) {
            THROW("Unable to create GL Context")
        }

        GLint major = 0;
        GLint minor = 0;
        glGetIntegerv(GL_MAJOR_VERSION, &major);
        glGetIntegerv(GL_MINOR_VERSION, &minor);

        const XrVersion minApiVersionSupported = XR_MAKE_VERSION(major, minor, 0);
        LOGI("###### OpenGL ES version %d.%d is supported, minXrVersion: %lu", major, minor,
             graphicsRequirements.minApiVersionSupported);
        if (graphicsRequirements.minApiVersionSupported > minApiVersionSupported) {
            THROW("Minimum required OpenGL ES version is %d.%d, but runtime reports %d.%d is supported")
        }

        m_contextApiMajorVersion = major;
        m_graphicsBinding.display = window.display;
        m_graphicsBinding.config = (EGLConfig) 0;
        m_graphicsBinding.context = window.context.context;

        glEnable(GL_DEBUG_OUTPUT);
        // init resources
    }

    void initialize_actions(){
        XrActionSetCreateInfo actionSetCreateInfo{XR_TYPE_ACTION_SET_CREATE_INFO};
        strcpy_s(actionSetCreateInfo.actionSetName, "gameplay");
        strcpy_s(actionSetCreateInfo.localizedActionSetName, "Gameplay");
        actionSetCreateInfo.priority = 0;
        OPENXR_CHECK(xrCreateActionSet(m_xrInstance, &actionSetCreateInfo, &m_input.actionSet),
                     "Failed to create action set.")


        OPENXR_CHECK(xrStringToPath(m_xrInstance, "/user/hand/left", &m_input.handSubactionPath[Side::LEFT]),"failed instance left hand")
        OPENXR_CHECK(xrStringToPath(m_xrInstance, "/user/hand/right", &m_input.handSubactionPath[Side::RIGHT]),"failed instance right hand")

        // Create actions
        {
            XrActionCreateInfo actionInfo{XR_TYPE_ACTION_CREATE_INFO};
            actionInfo.actionType = XR_ACTION_TYPE_FLOAT_INPUT;
            strcpy_s(actionInfo.actionName, "grab_object");
            strcpy_s(actionInfo.localizedActionName, "Grab Object");
            actionInfo.countSubactionPaths = uint32_t(m_input.handSubactionPath.size());
            actionInfo.subactionPaths = m_input.handSubactionPath.data();
            OPENXR_CHECK(xrCreateAction(m_input.actionSet, &actionInfo, &m_input.grabAction), "failed create grab object action")


            actionInfo.actionType = XR_ACTION_TYPE_POSE_INPUT;
            strcpy_s(actionInfo.actionName, "hand_pose");
            strcpy_s(actionInfo.localizedActionName, "Hand Pose");
            actionInfo.countSubactionPaths = uint32_t(m_input.handSubactionPath.size());
            actionInfo.subactionPaths = m_input.handSubactionPath.data();
            OPENXR_CHECK(xrCreateAction(m_input.actionSet, &actionInfo, &m_input.poseAction), "failed create pose action")

            actionInfo.actionType = XR_ACTION_TYPE_BOOLEAN_INPUT;
            strcpy_s(actionInfo.actionName, "quit_session");
            strcpy_s(actionInfo.localizedActionName, "Quit Session");
            actionInfo.countSubactionPaths = 0;
            actionInfo.subactionPaths = nullptr;
            OPENXR_CHECK(xrCreateAction(m_input.actionSet, &actionInfo, &m_input.quitAction), "failed create quit action");
        }

        // select path
        std::array<XrPath, Side::COUNT> selectPath;
        std::array<XrPath, Side::COUNT> squeezeValuePath;
        std::array<XrPath, Side::COUNT> squeezeForcePath;
        std::array<XrPath, Side::COUNT> squeezeClickPath;
        std::array<XrPath, Side::COUNT> posePath;
        std::array<XrPath, Side::COUNT> hapticPath;
        std::array<XrPath, Side::COUNT> menuClickPath;
        std::array<XrPath, Side::COUNT> bClickPath;
        std::array<XrPath, Side::COUNT> triggerValuePath;

        OPENXR_CHECK(xrStringToPath(m_xrInstance, "/user/hand/left/input/select/click", &selectPath[Side::LEFT]),"failed left click")
        OPENXR_CHECK(xrStringToPath(m_xrInstance, "/user/hand/right/input/select/click", &selectPath[Side::RIGHT]),"failed right click")
        OPENXR_CHECK(xrStringToPath(m_xrInstance, "/user/hand/left/input/squeeze/value", &squeezeValuePath[Side::LEFT]),"failed right click")
        OPENXR_CHECK(xrStringToPath(m_xrInstance, "/user/hand/right/input/squeeze/value", &squeezeValuePath[Side::RIGHT]),"failed right click")
        OPENXR_CHECK(xrStringToPath(m_xrInstance, "/user/hand/left/input/squeeze/force", &squeezeForcePath[Side::LEFT]),"failed right click")
        OPENXR_CHECK(xrStringToPath(m_xrInstance, "/user/hand/right/input/squeeze/force", &squeezeForcePath[Side::RIGHT]),"failed right click")
        OPENXR_CHECK(xrStringToPath(m_xrInstance, "/user/hand/left/input/squeeze/click", &squeezeClickPath[Side::LEFT]),"failed right click")
        OPENXR_CHECK(xrStringToPath(m_xrInstance, "/user/hand/right/input/squeeze/click", &squeezeClickPath[Side::RIGHT]),"failed right click")
        OPENXR_CHECK(xrStringToPath(m_xrInstance, "/user/hand/left/input/grip/pose", &posePath[Side::LEFT]),"failed right click")
        OPENXR_CHECK(xrStringToPath(m_xrInstance, "/user/hand/right/input/grip/pose", &posePath[Side::RIGHT]),"failed right click")
        OPENXR_CHECK(xrStringToPath(m_xrInstance, "/user/hand/left/output/haptic", &hapticPath[Side::LEFT]),"failed right click")
        OPENXR_CHECK(xrStringToPath(m_xrInstance, "/user/hand/right/output/haptic", &hapticPath[Side::RIGHT]),"failed right click")
        OPENXR_CHECK(xrStringToPath(m_xrInstance, "/user/hand/left/input/menu/click", &menuClickPath[Side::LEFT]),"failed right click")
        OPENXR_CHECK(xrStringToPath(m_xrInstance, "/user/hand/right/input/menu/click", &menuClickPath[Side::RIGHT]),"failed right click")
        OPENXR_CHECK(xrStringToPath(m_xrInstance, "/user/hand/left/input/b/click", &bClickPath[Side::LEFT]),"failed right click")
        OPENXR_CHECK(xrStringToPath(m_xrInstance, "/user/hand/right/input/b/click", &bClickPath[Side::RIGHT]),"failed right click")
        OPENXR_CHECK(xrStringToPath(m_xrInstance, "/user/hand/left/input/trigger/value", &triggerValuePath[Side::LEFT]),"failed right click")
        OPENXR_CHECK(xrStringToPath(m_xrInstance, "/user/hand/right/input/trigger/value", &triggerValuePath[Side::RIGHT]),"failed right click")

        // Suggest bindings for KHR Simple.
        {

        }
    }

    void initialize_session() {
        LOGI("###### initialize session");
        XrSessionCreateInfo sessionCreateInfo{XR_TYPE_SESSION_CREATE_INFO};
        sessionCreateInfo.next = (XrBaseInStructure *) &m_graphicsBinding;
        sessionCreateInfo.systemId = m_systemId;
        OPENXR_CHECK(xrCreateSession(m_xrInstance, &sessionCreateInfo, &m_session),
                     "Failed to create session.")

        uint32_t spaceCount;
        OPENXR_CHECK(xrEnumerateReferenceSpaces(m_session, 0, &spaceCount, nullptr),
                     "Failed to enumerate reference spaces.")
        std::vector<XrReferenceSpaceType> spaces(spaceCount);
        OPENXR_CHECK(xrEnumerateReferenceSpaces(m_session, (uint32_t) spaces.size(), &spaceCount,
                                                spaces.data()),
                     "Failed to enumerate reference spaces.")

        LOGI("###### reference spaces: %d", spaceCount);
        for (XrReferenceSpaceType space: spaces) {
            LOGI("###### reference name: %s", to_string(space));
        }

        // actions
        initialize_actions();
        create_visualized_spaces();

        {
            XrReferenceSpaceCreateInfo referenceSpaceCreateInfo = GetXrReferenceSpaceCreateInfo(
                    "Local");
            OPENXR_CHECK(xrCreateReferenceSpace(m_session, &referenceSpaceCreateInfo, &m_appSpace),
                         "Failed to create reference space Local.")
        }
    }

    void render_view(const XrCompositionLayerProjectionView &layerView,
                     const XrSwapchainImageBaseHeader *swapchainImage,
                     int64_t swapchainFormat, const std::vector<Cube> &cubes) {
        UNUSED_PARM(swapchainFormat)
        glBindFramebuffer(GL_FRAMEBUFFER, m_swapchainFramebuffer);

        const uint32_t colorTexture = reinterpret_cast<const XrSwapchainImageOpenGLESKHR *>(swapchainImage)->image;

        glViewport(static_cast<GLint>(layerView.subImage.imageRect.offset.x),
                   static_cast<GLint>(layerView.subImage.imageRect.offset.y),
                   static_cast<GLsizei>(layerView.subImage.imageRect.extent.width),
                   static_cast<GLsizei>(layerView.subImage.imageRect.extent.height));

        glFrontFace(GL_CW);
        glCullFace(GL_BACK);
        glEnable(GL_CULL_FACE);
        glEnable(GL_DEPTH_TEST);

        const uint32_t depthTexture = GetDepthTexture(colorTexture);

        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, colorTexture,
                               0);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthTexture, 0);
        // Clear swapchain and depth buffer.
        glClearColor(m_clearColor[0], m_clearColor[1], m_clearColor[2], m_clearColor[3]);
        glClearDepthf(1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
        // Set shaders and uniform variables.
        glUseProgram(m_program);

        const auto &pose = layerView.pose;
        XrMatrix4x4f proj;
        XrMatrix4x4f_CreateProjectionFov(&proj, GRAPHICS_OPENGL_ES, layerView.fov, 0.05f, 100.0f);
        XrMatrix4x4f toView;
        XrVector3f scale{1.f, 1.f, 1.f};
        XrMatrix4x4f_CreateTranslationRotationScale(&toView, &pose.position, &pose.orientation,
                                                    &scale);
        XrMatrix4x4f view;
        XrMatrix4x4f_InvertRigidBody(&view, &toView);
        XrMatrix4x4f vp;
        XrMatrix4x4f_Multiply(&vp, &proj, &view);

        // Set cube primitive data.
        glBindVertexArray(m_vao);

        // Render each cube
        for (const Cube &cube: cubes) {
            // Compute the model-view-projection transform and set it..
            XrMatrix4x4f model;
            XrMatrix4x4f_CreateTranslationRotationScale(&model, &cube.Pose.position,
                                                        &cube.Pose.orientation, &cube.Scale);
            XrMatrix4x4f mvp;
            XrMatrix4x4f_Multiply(&mvp, &vp, &model);
            glUniformMatrix4fv(m_modelViewProjectionUniformLocation, 1, GL_FALSE,
                               reinterpret_cast<const GLfloat *>(&mvp));

            // Draw the cube.
            glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(ArraySize(Geometry::c_cubeIndices)),
                           GL_UNSIGNED_SHORT, nullptr);
        }

        glBindVertexArray(0);
        glUseProgram(0);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    bool render_layer(XrTime predictedDisplayTime,
                      std::vector<XrCompositionLayerProjectionView> &projectionLayerViews,
                      XrCompositionLayerProjection &layer) {

        XrResult res;

        XrViewState viewState{XR_TYPE_VIEW_STATE};
        uint32_t viewCapacity = (uint32_t) m_views.size();
        uint32_t viewCountOutput;

        XrViewLocateInfo viewLocateInfo{XR_TYPE_VIEW_LOCATE_INFO};
        viewLocateInfo.viewConfigurationType = viewConfigType;
        viewLocateInfo.displayTime = predictedDisplayTime;
        viewLocateInfo.space = m_appSpace;

        res = xrLocateViews(m_session, &viewLocateInfo, &viewState, viewCapacity, &viewCountOutput,
                            m_views.data());
        CHECK_XRRESULT(res, "xrLocateViews")
        if ((viewState.viewStateFlags & XR_VIEW_STATE_POSITION_VALID_BIT) == 0 ||
            (viewState.viewStateFlags & XR_VIEW_STATE_ORIENTATION_VALID_BIT) == 0) {
            return false;
        }

        projectionLayerViews.resize(viewCountOutput);

        // render cube
        std::vector<Cube> cubes;
        for (XrSpace visualizedSpace: m_visualizedSpaces) {
            XrSpaceLocation spaceLocation{XR_TYPE_SPACE_LOCATION};
            res = xrLocateSpace(visualizedSpace, m_appSpace, predictedDisplayTime, &spaceLocation);
            CheckXrResult(res, "xrLocateSpace");
            if (XR_UNQUALIFIED_SUCCESS(res)) {
                if ((spaceLocation.locationFlags & XR_SPACE_LOCATION_POSITION_VALID_BIT) != 0 &&
                    (spaceLocation.locationFlags & XR_SPACE_LOCATION_ORIENTATION_VALID_BIT) != 0) {
                    cubes.push_back(Cube{spaceLocation.pose, {0.25f, 0.25f, 0.25f}});
                }
            } else {
                LOGI("###### Unable to locate a visualized reference space in app space: %d", res);
            }
        }

        // render hand

        // render view
        for (uint32_t i = 0; i < viewCountOutput; i++) {
            const Swapchain viewSwapchain = m_swapchains[i];

            // get image
            XrSwapchainImageAcquireInfo acquireInfo{XR_TYPE_SWAPCHAIN_IMAGE_ACQUIRE_INFO};

            uint32_t swapchainIndex;
            OPENXR_CHECK(xrAcquireSwapchainImage(viewSwapchain.handle, &acquireInfo,
                                                 &swapchainIndex),
                         "Failed to acquire swapchain image.")

            XrSwapchainImageWaitInfo waitInfo{XR_TYPE_SWAPCHAIN_IMAGE_WAIT_INFO};
            waitInfo.timeout = XR_INFINITE_DURATION;
            OPENXR_CHECK(xrWaitSwapchainImage(viewSwapchain.handle, &waitInfo),
                         "Failed to wait swapchain image.")

            projectionLayerViews[i] = {XR_TYPE_COMPOSITION_LAYER_PROJECTION_VIEW};
            projectionLayerViews[i].pose = m_views[i].pose;
            projectionLayerViews[i].fov = m_views[i].fov;
            projectionLayerViews[i].subImage.swapchain = viewSwapchain.handle;
            projectionLayerViews[i].subImage.imageRect.offset = {0, 0};
            projectionLayerViews[i].subImage.imageRect.extent = {viewSwapchain.width,
                                                                 viewSwapchain.height};

            const XrSwapchainImageBaseHeader *const swapchainImage = m_swapchainImages[viewSwapchain.handle][swapchainIndex];
            render_view(projectionLayerViews[i], swapchainImage, m_colorSwapchainFormat, cubes);

            XrSwapchainImageReleaseInfo releaseInfo{XR_TYPE_SWAPCHAIN_IMAGE_RELEASE_INFO};
            OPENXR_CHECK(xrReleaseSwapchainImage(viewSwapchain.handle, &releaseInfo),
                         "Failed to release swapchain image.")
        }

        //TODO important
        layer.space = m_appSpace;
        layer.layerFlags = environmentBlendMode == XR_ENVIRONMENT_BLEND_MODE_ALPHA_BLEND
                           ? XR_COMPOSITION_LAYER_BLEND_TEXTURE_SOURCE_ALPHA_BIT |
                             XR_COMPOSITION_LAYER_UNPREMULTIPLIED_ALPHA_BIT : 0;
        layer.viewCount = (uint32_t) projectionLayerViews.size();
        layer.views = projectionLayerViews.data();
        return true;
    }

    void render_frame() {
        XrFrameWaitInfo frameWaitInfo{XR_TYPE_FRAME_WAIT_INFO};
        XrFrameState frameState{XR_TYPE_FRAME_STATE};
        OPENXR_CHECK(xrWaitFrame(m_session, &frameWaitInfo, &frameState),
                     "Failed to wait frame.")

        XrFrameBeginInfo frameBeginInfo{XR_TYPE_FRAME_BEGIN_INFO};
        OPENXR_CHECK(xrBeginFrame(m_session, &frameBeginInfo), "Failed to begin frame.")

        std::vector<XrCompositionLayerBaseHeader *> layers;
        XrCompositionLayerProjection layer{XR_TYPE_COMPOSITION_LAYER_PROJECTION};
        std::vector<XrCompositionLayerProjectionView> projectionLayerViews;
        if (frameState.shouldRender == XR_TRUE) {
            // render layer
            if (render_layer(frameState.predictedDisplayTime, projectionLayerViews, layer)) {
//                LOGW("###### render_frame %ld", frameState.predictedDisplayTime);
                layers.push_back(reinterpret_cast<XrCompositionLayerBaseHeader *>(&layer));
            }
        }

        XrFrameEndInfo frameEndInfo{XR_TYPE_FRAME_END_INFO};
        frameEndInfo.displayTime = frameState.predictedDisplayTime;
        frameEndInfo.environmentBlendMode = environmentBlendMode;
        frameEndInfo.layerCount = (uint32_t) layers.size();
        frameEndInfo.layers = layers.data();
        OPENXR_CHECK(xrEndFrame(m_session, &frameEndInfo), "Failed to end frame.")
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
        PFN_xrInitializeLoaderKHR initializeLoader = nullptr;
        OPENXR_CHECK(xrGetInstanceProcAddr(XR_NULL_HANDLE, "xrInitializeLoaderKHR",
                                           (PFN_xrVoidFunction *) &initializeLoader),
                     "Failed to get InstanceProcAddr for xrInitializeLoaderKHR.")

        if (!initializeLoader) {
            LOGE("Failed to get InstanceProcAddr for xrInitializeLoaderKHR.");
            return;
        }

        XrLoaderInitInfoAndroidKHR loaderInitInfoAndroid = {XR_TYPE_LOADER_INIT_INFO_ANDROID_KHR};
        loaderInitInfoAndroid.applicationVM = app->activity->vm;
        loaderInitInfoAndroid.applicationContext = app->activity->clazz;
        initializeLoader((const XrLoaderInitInfoBaseHeaderKHR *) &loaderInitInfoAndroid);

        // Create OpenXR Instance
        create_openxr_instance();
        initialize_system();
        initialize_device();
        initialize_resources();
        initialize_session();

        create_swapchains();


        LOGI("###### Enter Loop Render");
        // loop
        while (app->destroyRequested == 0) {
            // read all pending events;
            for (;;) {
                int events;
                struct android_poll_source *source;
                const int timeoutMilliseconds = (!appState.Resumed && IsSessionRunning() &&
                                                 app->destroyRequested == 0) ? -1 : 0;
                if (ALooper_pollAll(timeoutMilliseconds, nullptr, &events, (void **) &source) < 0) {
                    break;
                }

                // process the event
                if (source != nullptr) {
                    source->process(app, source);
                }
            }

            // poll events
            PollEvents(&exitRenderLoop, &requestRestart);
            if (exitRenderLoop) {
                ANativeActivity_finish(app->activity);
                continue;
            }

            if (!IsSessionRunning()) {
                std::this_thread::sleep_for(std::chrono::milliseconds(200));
                continue;
            }

            // pollaction
            render_frame();
        }

        LOGI("###### DetachCurrentThread");
        app->activity->vm->DetachCurrentThread();
    }
}