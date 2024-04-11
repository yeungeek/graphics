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
#include "common/openxr_debugutils.h"
#include "common/geometry.h"
#include "common/xr_linear_algebra.h"

#include <common/gfxwrapper_opengl.h>

#include <android/log.h>
#include <android_native_app_glue.h>
#include <android/native_window.h>
#include <jni.h>
#include <sys/system_properties.h>

namespace Chapter3 {
    XrVector3f operator-(XrVector3f a, XrVector3f b) {
        return {a.x - b.x, a.y - b.y, a.z - b.z};
    }
    XrVector3f operator*(XrVector3f a, float b) {
        return {a.x * b, a.y * b, a.z * b};
    }

    // The version statement has come on first line.
    static const char* VertexShaderGlsl = R"_(#version 320 es

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
    static const char* FragmentShaderGlsl = R"_(#version 320 es

    in lowp vec3 PSVertexColor;
    out lowp vec4 FragColor;

    void main() {
       FragColor = vec4(PSVertexColor, 1);
    }
    )_";
//class
    class OpenXRTutorial {
    public:
        OpenXRTutorial() {
        }

        ~OpenXRTutorial() = default;

        void Run() {
            Log::Write(Log::Level::Info, "###### OpenXR Tutorial Run");
            CreateInstance();
            CreateDebugMessenger();

            GetInstanceProperties();
            GetSystemID();

            InitializeDevice();
            GetViewConfigurations();
            GetEnvironmentBlendMode();

            CreateSession();
            CreateReferenceSpace();
            CreateSwapchains();
            CreateResources();

            // poll
            while (m_applicationRunning) {
                PollSystemEvents();
                PollEvents();
                if (m_sessionRunning) {
                    //render
                    RenderFrame();
                }
            }

            DestroyResources();
            DestroySwapchains();
            DestroyReferenceSpace();
            DestroySession();

            DestroyDebugMessenger();
            DestroyInstance();
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

        enum class SwapchainType : uint8_t {
            COLOR,
            DEPTH
        };

        struct RenderLayerInfo {
            XrTime predictedDisplayTime = 0;
            std::vector<XrCompositionLayerBaseHeader *> layers;
            XrCompositionLayerProjection layerProjection = {XR_TYPE_COMPOSITION_LAYER_PROJECTION};
            std::vector<XrCompositionLayerProjectionView> layerProjectionViews;
        };

        struct ImageViewCreateInfo {
            void* image;
            enum class Type : uint8_t {
                RTV,
                DSV,
                SRV,
                UAV
            } type;
            enum class View : uint8_t {
                TYPE_1D,
                TYPE_2D,
                TYPE_3D,
                TYPE_CUBE,
                TYPE_1D_ARRAY,
                TYPE_2D_ARRAY,
                TYPE_CUBE_ARRAY,
            } view;
            int64_t format;
            enum class Aspect : uint8_t {
                COLOR_BIT = 0x01,
                DEPTH_BIT = 0x02,
                STENCIL_BIT = 0x04
            } aspect;
            uint32_t baseMipLevel;
            uint32_t levelCount;
            uint32_t baseArrayLayer;
            uint32_t layerCount;
        };

        struct Viewport {
            float x;
            float y;
            float width;
            float height;
            float minDepth;
            float maxDepth;
        };
        struct Offset2D {
            int32_t x;
            int32_t y;
        };
        struct Extent2D {
            uint32_t width;
            uint32_t height;
        };
        struct Rect2D {
            Offset2D offset;
            Extent2D extent;
        };
        struct BufferCreateInfo {
            enum class Type : uint8_t {
                VERTEX,
                INDEX,
                UNIFORM,
            } type;
            size_t stride;
            size_t size;
            void* data;
        };

        struct ShaderCreateInfo {
            enum class Type : uint8_t {
                VERTEX,
                TESSELLATION_CONTROL,
                TESSELLATION_EVALUATION,
                GEOMETRY,
                FRAGMENT,
                COMPUTE
            } type;
            const char* sourceData;
            size_t sourceSize;
        };

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
        // XR_DOCS_TAG_END_Android_System_Functionality

    private:
        void CreateInstance() {
            //Android create instance
            std::vector<const char*> extensions;
            const std::vector<std::string> platformExtensions = {XR_KHR_ANDROID_CREATE_INSTANCE_EXTENSION_NAME};
            std::transform(platformExtensions.begin(), platformExtensions.end(), std::back_inserter(extensions),
                           [](const std::string& ext) { return ext.c_str(); });

            const std::vector<std::string> graphicsExtensions = {XR_KHR_OPENGL_ES_ENABLE_EXTENSION_NAME};
            std::transform(graphicsExtensions.begin(), graphicsExtensions.end(), std::back_inserter(extensions),
                           [](const std::string& ext) { return ext.c_str(); });

            //2. Create Instance
            XrInstanceCreateInfo instanceCI{XR_TYPE_INSTANCE_CREATE_INFO};
            XrInstanceCreateInfoAndroidKHR instanceCreateInfoAndroid{XR_TYPE_INSTANCE_CREATE_INFO_ANDROID_KHR};
            instanceCreateInfoAndroid.applicationVM = androidApp->activity->vm;
            instanceCreateInfoAndroid.applicationActivity = androidApp->activity->clazz;

            instanceCI.next = (XrBaseInStructure*)&instanceCreateInfoAndroid;
            instanceCI.enabledExtensionCount = (uint32_t)extensions.size();
            instanceCI.enabledExtensionNames = extensions.data();

            strcpy(instanceCI.applicationInfo.applicationName, "HelloXR");
            instanceCI.applicationInfo.apiVersion = XR_CURRENT_API_VERSION;

            OPENXR_CHECK(xrCreateInstance(&instanceCI, &m_xrInstance), "Failed to create Instance.")
        }

        void CreateDebugMessenger(){
            if (IsStringInVector(m_activeInstanceExtensions, XR_EXT_DEBUG_UTILS_EXTENSION_NAME)) {
                m_debugUtilsMessenger = CreateOpenXRDebugUtilsMessenger(m_xrInstance);
            }
        }

        void GetViewConfigurations() {
            Log::Write(Log::Level::Info,"###### GetViewConfigurations Start");
            uint32_t viewConfigTypeCount = 0;
            OPENXR_CHECK(xrEnumerateViewConfigurations(m_xrInstance, m_systemID, 0, &viewConfigTypeCount, nullptr),
                           "Failed to get view configuration type count.")
            m_viewConfigurations.resize(viewConfigTypeCount);
            OPENXR_CHECK(xrEnumerateViewConfigurations(m_xrInstance, m_systemID, viewConfigTypeCount, &viewConfigTypeCount, m_viewConfigurations.data()),
                           "Failed to get view configurations.")

            for(const XrViewConfigurationType& viewConfigType : m_applicationViewConfigurations){
                if(std::find(m_viewConfigurations.begin(), m_viewConfigurations.end(), viewConfigType) != m_viewConfigurations.end()){
                    m_viewConfiguration = viewConfigType;
                    break;
                }
            }

            if(m_viewConfiguration == XR_VIEW_CONFIGURATION_TYPE_MAX_ENUM){
                Log::Write(Log::Level::Error, "No supported view configuration types found.");
                m_viewConfiguration = XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO;
            }

            uint32_t viewConfigurationViewCount = 0;
            OPENXR_CHECK(xrEnumerateViewConfigurationViews(m_xrInstance, m_systemID, m_viewConfiguration, 0, &viewConfigurationViewCount, nullptr),
                           "Failed to get view configuration view count.")
            m_viewConfigurationViews.resize(viewConfigurationViewCount,{XR_TYPE_VIEW_CONFIGURATION_VIEW});
            OPENXR_CHECK(xrEnumerateViewConfigurationViews(m_xrInstance, m_systemID, m_viewConfiguration, viewConfigurationViewCount, &viewConfigurationViewCount, m_viewConfigurationViews.data()),
                           "Failed to get view configuration views.")

            Log::Write(Log::Level::Info,Fmt("###### GetViewConfigurations: %d", m_viewConfiguration));
        }

        void GetEnvironmentBlendMode(){
            Log::Write(Log::Level::Info,"###### GetEnvironmentBlendMode Start");
            uint32_t environmentBlendModeCount = 0;
            OPENXR_CHECK(xrEnumerateEnvironmentBlendModes(m_xrInstance, m_systemID, m_viewConfiguration, 0, &environmentBlendModeCount, &m_environmentBlendMode),
                           "Failed to get environment blend mode count.")
            m_environmentBlendModes.resize(environmentBlendModeCount);
            OPENXR_CHECK(xrEnumerateEnvironmentBlendModes(m_xrInstance, m_systemID, m_viewConfiguration, environmentBlendModeCount, &environmentBlendModeCount, m_environmentBlendModes.data()),
                           "Failed to get environment blend modes.")

            for(const XrEnvironmentBlendMode& environmentBlendMode : m_applicationEnvironmentBlendModes){
                if(std::find(m_environmentBlendModes.begin(), m_environmentBlendModes.end(), environmentBlendMode) != m_environmentBlendModes.end()){
                    m_environmentBlendMode = environmentBlendMode;
                    break;
                }
            }

            if(m_environmentBlendMode == XR_ENVIRONMENT_BLEND_MODE_MAX_ENUM){
                Log::Write(Log::Level::Error, "No supported environment blend modes found.");
                m_environmentBlendMode = XR_ENVIRONMENT_BLEND_MODE_OPAQUE;
            }
        }

        void InitializeDevice(){
            Log::Write(Log::Level::Info,"###### InitializeDevice Start");
            PFN_xrGetOpenGLESGraphicsRequirementsKHR pfnGetOpenGLESGraphicsRequirementsKHR = nullptr;
            OPENXR_CHECK(xrGetInstanceProcAddr(m_xrInstance, "xrGetOpenGLESGraphicsRequirementsKHR",
                                                 reinterpret_cast<PFN_xrVoidFunction*>(&pfnGetOpenGLESGraphicsRequirementsKHR)),
                           "Failed to get xrGetOpenGLESGraphicsRequirementsKHR function pointer.")
            XrGraphicsRequirementsOpenGLESKHR graphicsRequirements{XR_TYPE_GRAPHICS_REQUIREMENTS_OPENGL_ES_KHR};
            OPENXR_CHECK(pfnGetOpenGLESGraphicsRequirementsKHR(m_xrInstance, m_systemID, &graphicsRequirements),
                           "Failed to get OpenGL ES graphics requirements.")

            Log::Write(Log::Level::Info,"###### InitializeDevice Starting");
            // Init the gl extensions
            ksDriverInstance driverInstance{};
            ksGpuQueueInfo queueInfo{};

            ksGpuSurfaceColorFormat colorFormat{KS_GPU_SURFACE_COLOR_FORMAT_B8G8R8A8};
            ksGpuSurfaceDepthFormat depthFormat{KS_GPU_SURFACE_DEPTH_FORMAT_D24};
            ksGpuSampleCount sampleCount{KS_GPU_SAMPLE_COUNT_1};
            if (!ksGpuWindow_Create(&window, &driverInstance, &queueInfo, 0, colorFormat, depthFormat, sampleCount, 640, 480, false)) {
                THROW("Unable to create GL context");
            }

            GLint major = 0;
            GLint minor = 0;
            glGetIntegerv(GL_MAJOR_VERSION, &major);
            glGetIntegerv(GL_MINOR_VERSION, &minor);

            const XrVersion desiredApiVersion = XR_MAKE_VERSION(major, minor, 0);
            if(graphicsRequirements.minApiVersionSupported > desiredApiVersion){
                THROW("Runtime does not support desired Graphics API and/or version")
            }

            m_contextApiMajorVersion = major;

            m_graphicsBinding.display = window.display;
            m_graphicsBinding.config = (EGLConfig)0;
            m_graphicsBinding.context = window.context.context;

            glEnable(GL_DEBUG_OUTPUT);
//            glDebugMessageCallback(
//                    [](GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* message,
//                       const void* userParam) {
//                        Log::Write(Log::Level::Info, "###### GLES Debug: " + std::string(message, 0, length));
//                    },
//                    this);

            // Initialize the resources
//            InitializeResources();
        }

        void BeginRendering() {
            glGenVertexArrays(1, &vertexArray);
            glBindVertexArray(vertexArray);

            glGenFramebuffers(1, &setFramebuffer);
            glBindFramebuffer(GL_FRAMEBUFFER, setFramebuffer);
        }

        void EndRendering() {
            glBindFramebuffer(GL_FRAMEBUFFER, 0);
            glDeleteFramebuffers(1, &setFramebuffer);
            setFramebuffer = 0;

            glBindVertexArray(0);
            glDeleteVertexArrays(1, &vertexArray);
            vertexArray = 0;
        }

        void ClearColor(void *imageView, float r, float g, float b, float a) {
            glBindFramebuffer(GL_FRAMEBUFFER, (GLuint)(uint64_t)imageView);
            glClearColor(r, g, b, a);
            glClear(GL_COLOR_BUFFER_BIT);
            glBindFramebuffer(GL_FRAMEBUFFER, 0);
        }

        void ClearDepth(void *imageView, float d) {
            glBindFramebuffer(GL_FRAMEBUFFER, (GLuint)(uint64_t)imageView);
            glClearDepthf(d);
            glClear(GL_DEPTH_BUFFER_BIT);
            glBindFramebuffer(GL_FRAMEBUFFER, 0);
        }

        void *CreateShader(const ShaderCreateInfo &shaderCI) {
            GLenum type = 0;
            switch (shaderCI.type) {
                case ShaderCreateInfo::Type::VERTEX: {
                    type = GL_VERTEX_SHADER;
                    break;
                }
                case ShaderCreateInfo::Type::TESSELLATION_CONTROL: {
                    type = GL_TESS_CONTROL_SHADER;
                    break;
                }
                case ShaderCreateInfo::Type::TESSELLATION_EVALUATION: {
                    type = GL_TESS_EVALUATION_SHADER;
                    break;
                }
                case ShaderCreateInfo::Type::GEOMETRY: {
                    type = GL_GEOMETRY_SHADER;
                    break;
                }
                case ShaderCreateInfo::Type::FRAGMENT: {
                    type = GL_FRAGMENT_SHADER;
                    break;
                }
                case ShaderCreateInfo::Type::COMPUTE: {
                    type = GL_COMPUTE_SHADER;
                    break;
                }
                default:
                    std::cout << "ERROR: OPENGL: Unknown Shader Type." << std::endl;
            }
            GLuint shader = glCreateShader(type);

            glShaderSource(shader, 1, &shaderCI.sourceData, nullptr);
            glCompileShader(shader);

            GLint isCompiled = 0;
            glGetShaderiv(shader, GL_COMPILE_STATUS, &isCompiled);
            if (isCompiled == GL_FALSE) {
                GLint maxLength = 0;
                glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &maxLength);

                std::vector<GLchar> infoLog(maxLength);
                glGetShaderInfoLog(shader, maxLength, &maxLength, &infoLog[0]);
                std::cout << infoLog.data() << std::endl;
                DEBUG_BREAK;

                glDeleteShader(shader);
                shader = 0;
            }
            return (void *)(uint64_t)shader;
        }


        void CreateSession() {
            //3. CreateSession
            Log::Write(Log::Level::Info,"###### CreateSession Start");
            XrSessionCreateInfo sessionCI{XR_TYPE_SESSION_CREATE_INFO};
            // graphics
            sessionCI.createFlags = 0;
            sessionCI.next = reinterpret_cast<const XrBaseInStructure*>(&m_graphicsBinding);
            sessionCI.systemId = m_systemID;
            OPENXR_CHECK(xrCreateSession(m_xrInstance, &sessionCI, &m_session), "Failed to create Session.")
        }

        void CreateReferenceSpace() {
            Log::Write(Log::Level::Info,"###### CreateReferenceSpace Start");
            XrReferenceSpaceCreateInfo referenceSpaceCI{XR_TYPE_REFERENCE_SPACE_CREATE_INFO};
            referenceSpaceCI.referenceSpaceType = XR_REFERENCE_SPACE_TYPE_LOCAL;
            referenceSpaceCI.poseInReferenceSpace = {{0.0f,0.0f,0.0f,1.0f},{0.0f,0.0f,0.0f}};
            OPENXR_CHECK(xrCreateReferenceSpace(m_session, &referenceSpaceCI, &m_localSpace), "Failed to create Reference Space.")
        }

        void CreateSwapchains() {
            Log::Write(Log::Level::Info,"###### CreateSwapChains Start");

            uint32_t formatCount = 0;
            OPENXR_CHECK(xrEnumerateSwapchainFormats(m_session, 0, &formatCount, nullptr), "Failed to get Swapchain Format Count.")
            std::vector<int64_t> formats(formatCount);
            OPENXR_CHECK(xrEnumerateSwapchainFormats(m_session, formatCount, &formatCount, formats.data()), "Failed to get Swapchain Formats.")
            Log::Write(Log::Level::Info, Fmt("###### Swapchain Formats: %d", formatCount));

            m_colorSwapchainInfos.resize(m_viewConfigurations.size());
            m_depthSwapchainInfos.resize(m_viewConfigurations.size());

            for(size_t i = 0; i < m_viewConfigurationViews.size(); i++){
                SwapchainInfo &colorSwapchainInfo = m_colorSwapchainInfos[i];
                SwapchainInfo &depthSwapchainInfo = m_depthSwapchainInfos[i];

                // create xrswapchain
                // color
                XrSwapchainCreateInfo swapchainCI{XR_TYPE_SWAPCHAIN_CREATE_INFO};
                swapchainCI.createFlags = 0;
                swapchainCI.usageFlags = XR_SWAPCHAIN_USAGE_SAMPLED_BIT | XR_SWAPCHAIN_USAGE_COLOR_ATTACHMENT_BIT;
                swapchainCI.format = GL_RGBA8;  //TODO type
                swapchainCI.sampleCount = m_viewConfigurationViews[i].recommendedSwapchainSampleCount;
                swapchainCI.width = m_viewConfigurationViews[i].recommendedImageRectWidth;
                swapchainCI.height = m_viewConfigurationViews[i].recommendedImageRectHeight;
                swapchainCI.faceCount = 1;
                swapchainCI.arraySize = 1;
                swapchainCI.mipCount = 1;
                OPENXR_CHECK(xrCreateSwapchain(m_session, &swapchainCI, &colorSwapchainInfo.swapchain), "Failed to create Color Swapchain.")
                colorSwapchainInfo.swapchainFormat = swapchainCI.format;

                //depth
                swapchainCI.createFlags = 0;
                swapchainCI.usageFlags = XR_SWAPCHAIN_USAGE_SAMPLED_BIT | XR_SWAPCHAIN_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
                swapchainCI.format = GL_DEPTH_COMPONENT32F; //TODO type
                swapchainCI.sampleCount = m_viewConfigurationViews[i].recommendedSwapchainSampleCount;
                swapchainCI.width = m_viewConfigurationViews[i].recommendedImageRectWidth;
                swapchainCI.height = m_viewConfigurationViews[i].recommendedImageRectHeight;
                swapchainCI.faceCount = 1;
                swapchainCI.arraySize = 1;
                swapchainCI.mipCount = 1;
                OPENXR_CHECK(xrCreateSwapchain(m_session, &swapchainCI, &depthSwapchainInfo.swapchain), "Failed to create Depth Swapchain.")
                depthSwapchainInfo.swapchainFormat = swapchainCI.format;

                uint32_t colorSwapchainImageCount = 0;
                OPENXR_CHECK(xrEnumerateSwapchainImages(colorSwapchainInfo.swapchain, 0, &colorSwapchainImageCount, nullptr), "Failed to get Color Swapchain Image Count.")
                XrSwapchainImageBaseHeader *colorSwapchainImages = AllocateSwapchainImageData(colorSwapchainInfo.swapchain, SwapchainType::COLOR, colorSwapchainImageCount);
                OPENXR_CHECK(xrEnumerateSwapchainImages(colorSwapchainInfo.swapchain, colorSwapchainImageCount, &colorSwapchainImageCount, colorSwapchainImages), "Failed to get Color Swapchain Images.")

                uint32_t depthSwapchainImageCount = 0;
                OPENXR_CHECK(xrEnumerateSwapchainImages(depthSwapchainInfo.swapchain, 0, &depthSwapchainImageCount, nullptr), "Failed to get Depth Swapchain Image Count.")
                XrSwapchainImageBaseHeader *depthSwapchainImages = AllocateSwapchainImageData(depthSwapchainInfo.swapchain, SwapchainType::DEPTH, depthSwapchainImageCount);
                OPENXR_CHECK(xrEnumerateSwapchainImages(depthSwapchainInfo.swapchain, depthSwapchainImageCount, &depthSwapchainImageCount, depthSwapchainImages), "Failed to get Depth Swapchain Images.")


                for(uint32_t j = 0; j < colorSwapchainImageCount; j++){
                    ImageViewCreateInfo imageViewCI;
                    imageViewCI.image = GetSwapchainImage(colorSwapchainInfo.swapchain,j);
                    imageViewCI.type = ImageViewCreateInfo::Type::RTV;
                    imageViewCI.view = ImageViewCreateInfo::View::TYPE_2D;
                    imageViewCI.format = colorSwapchainInfo.swapchainFormat;
                    imageViewCI.aspect = ImageViewCreateInfo::Aspect::COLOR_BIT;
                    imageViewCI.baseMipLevel = 0;
                    imageViewCI.levelCount = 1;
                    imageViewCI.baseArrayLayer = 0;
                    imageViewCI.layerCount = 1;
                    colorSwapchainInfo.imageViews.push_back(CreateImageView(imageViewCI));
                }

                for(uint32_t j = 0; j < depthSwapchainImageCount; i++){
                    ImageViewCreateInfo imageViewCI;
                    imageViewCI.image = GetSwapchainImage(depthSwapchainInfo.swapchain,j);
                    imageViewCI.type = ImageViewCreateInfo::Type::DSV;
                    imageViewCI.view = ImageViewCreateInfo::View::TYPE_2D;
                    imageViewCI.format = depthSwapchainInfo.swapchainFormat;
                    imageViewCI.aspect = ImageViewCreateInfo::Aspect::DEPTH_BIT;
                    imageViewCI.baseMipLevel = 0;
                    imageViewCI.levelCount = 1;
                    imageViewCI.baseArrayLayer = 0;
                    imageViewCI.layerCount = 1;
                    depthSwapchainInfo.imageViews.push_back(CreateImageView(imageViewCI));
                }
            }
        }

        void RenderFrame(){
            // get rendering info
            XrFrameState frameState{XR_TYPE_FRAME_STATE};
            XrFrameWaitInfo frameWaitInfo{XR_TYPE_FRAME_WAIT_INFO};
            OPENXR_CHECK(xrWaitFrame(m_session, &frameWaitInfo, &frameState),"Failed to wait for XR Frame")

            // beginning the frame
            XrFrameBeginInfo frameBeginInfo{XR_TYPE_FRAME_BEGIN_INFO};
            OPENXR_CHECK(xrBeginFrame(m_session, &frameBeginInfo), "Failed to begin XR Frame")

            //layer composition
            bool rendered = false;
            RenderLayerInfo renderLayerInfo;
            renderLayerInfo.predictedDisplayTime = frameState.predictedDisplayTime;

            // session check
            bool sessionActive = (m_sessionState == XR_SESSION_STATE_SYNCHRONIZED || m_sessionState == XR_SESSION_STATE_VISIBLE || m_sessionState == XR_SESSION_STATE_FOCUSED);
            if(sessionActive && frameState.shouldRender){
                // render stereo image
                rendered = RenderLayer(renderLayerInfo);
                if(rendered){
                    // submit layer
                    renderLayerInfo.layers.push_back(reinterpret_cast<XrCompositionLayerBaseHeader *>(&renderLayerInfo.layerProjection));
                }
            }

            // blending and layers
            XrFrameEndInfo frameEndInfo{XR_TYPE_FRAME_END_INFO};
            frameEndInfo.displayTime = frameState.predictedDisplayTime;
            frameEndInfo.environmentBlendMode = m_environmentBlendMode;
            frameEndInfo.layerCount = static_cast<uint32_t>(renderLayerInfo.layers.size());
            frameEndInfo.layers = renderLayerInfo.layers.data();
            OPENXR_CHECK(xrEndFrame(m_session, &frameEndInfo), "Failed to end XR Frame")
        }

        bool RenderLayer(RenderLayerInfo& renderLayerInfo){
            Log::Write(Log::Level::Warning,"###### RenderLayer");
            std::vector<XrView> views(m_viewConfigurationViews.size(),{XR_TYPE_VIEW});

            XrViewState viewState{XR_TYPE_VIEW_STATE};
            XrViewLocateInfo viewLocateInfo{XR_TYPE_VIEW_LOCATE_INFO};
            viewLocateInfo.viewConfigurationType = m_viewConfigurationType;
            viewLocateInfo.displayTime = renderLayerInfo.predictedDisplayTime;
            viewLocateInfo.space = m_localSpace;

            uint32_t viewCount = 0;
            XrResult  result = xrLocateViews(m_session, &viewLocateInfo, &viewState, (uint32_t)views.size(), &viewCount, views.data());
            if(result != XR_SUCCESS){
                Log::Write(Log::Level::Error,"###### Failed to locate view");
                return false;
            }

            renderLayerInfo.layerProjectionViews.resize(viewCount,{XR_TYPE_COMPOSITION_LAYER_PROJECTION_VIEW});

            for (uint32_t i = 0; i < viewCount; i++) {
                SwapchainInfo &colorSwapchainInfo = m_colorSwapchainInfos[i];
                SwapchainInfo &depthSwapchainInfo = m_depthSwapchainInfos[i];

                uint32_t colorImageIndex = 0;
                uint32_t depthImageIndex = 0;
                XrSwapchainImageAcquireInfo acquireInfo{XR_TYPE_SWAPCHAIN_IMAGE_ACQUIRE_INFO};
                OPENXR_CHECK(xrAcquireSwapchainImage(colorSwapchainInfo.swapchain, &acquireInfo, &colorImageIndex), "Failed to acquire color swapchain image.")
                OPENXR_CHECK(xrAcquireSwapchainImage(depthSwapchainInfo.swapchain, &acquireInfo, &depthImageIndex), "Failed to acquire depth swapchain image.")

                // get viewport and scissors
                const uint32_t &width = m_viewConfigurationViews[i].recommendedImageRectWidth;
                const uint32_t &height = m_viewConfigurationViews[i].recommendedImageRectHeight;

                Viewport viewport = {0.0f, 0.0f, (float)width, (float)height, 0.0f,1.0f};
                Rect2D scissor = {{0,0,},{width,height}};
                float nearZ = 0.05f;
                float farZ = 100.0f;

                // composition layer
                renderLayerInfo.layerProjectionViews[i] = {XR_TYPE_COMPOSITION_LAYER_PROJECTION_VIEW};
                renderLayerInfo.layerProjectionViews[i].pose = views[i].pose;
                renderLayerInfo.layerProjectionViews[i].fov = views[i].fov;
                renderLayerInfo.layerProjectionViews[i].subImage.swapchain = colorSwapchainInfo.swapchain;
                renderLayerInfo.layerProjectionViews[i].subImage.imageRect.offset.x = 0;
                renderLayerInfo.layerProjectionViews[i].subImage.imageRect.offset.y = 0;
                renderLayerInfo.layerProjectionViews[i].subImage.imageRect.extent.width = width;
                renderLayerInfo.layerProjectionViews[i].subImage.imageRect.extent.height = height;
                renderLayerInfo.layerProjectionViews[i].subImage.imageArrayIndex = 0;

                // render color and depth
                BeginRendering();
                if (m_environmentBlendMode == XR_ENVIRONMENT_BLEND_MODE_OPAQUE) {
                    ClearColor(colorSwapchainInfo.imageViews[colorImageIndex],0.17f,0.17f,0.17f,1.0f);
                } else {
                    ClearColor(colorSwapchainInfo.imageViews[colorImageIndex],0.0f,0.0f,0.0f,0.0f);
                }

                ClearDepth(depthSwapchainInfo.imageViews[depthImageIndex],1.0f);

                EndRendering();

                XrSwapchainImageReleaseInfo releaseInfo{XR_TYPE_SWAPCHAIN_IMAGE_RELEASE_INFO};
                OPENXR_CHECK(xrReleaseSwapchainImage(colorSwapchainInfo.swapchain, &releaseInfo), "Failed to release color swapchain image.")
                OPENXR_CHECK(xrReleaseSwapchainImage(depthSwapchainInfo.swapchain, &releaseInfo), "Failed to release depth swapchain image.")
            }

            // fill out
            renderLayerInfo.layerProjection.layerFlags =  XR_COMPOSITION_LAYER_BLEND_TEXTURE_SOURCE_ALPHA_BIT | XR_COMPOSITION_LAYER_CORRECT_CHROMATIC_ABERRATION_BIT;
            renderLayerInfo.layerProjection.space = m_localSpace;
            renderLayerInfo.layerProjection.viewCount = static_cast<uint32_t>(renderLayerInfo.layerProjectionViews.size());
            renderLayerInfo.layerProjection.views = renderLayerInfo.layerProjectionViews.data();

            return true;
        }

        void RenderCuboid(XrPosef pose, XrVector3f scale, XrVector3f color){

        }

        void CreateResources() {
            constexpr XrVector4f vertexPositions[] = {
                    {+0.5f, +0.5f, +0.5f, 1.0f},
                    {+0.5f, +0.5f, -0.5f, 1.0f},
                    {+0.5f, -0.5f, +0.5f, 1.0f},
                    {+0.5f, -0.5f, -0.5f, 1.0f},
                    {-0.5f, +0.5f, +0.5f, 1.0f},
                    {-0.5f, +0.5f, -0.5f, 1.0f},
                    {-0.5f, -0.5f, +0.5f, 1.0f},
                    {-0.5f, -0.5f, -0.5f, 1.0f}};

#define CUBE_FACE(V1, V2, V3, V4, V5, V6) vertexPositions[V1], vertexPositions[V2], vertexPositions[V3], vertexPositions[V4], vertexPositions[V5], vertexPositions[V6],
            XrVector4f cubeVertices[] = {
                    CUBE_FACE(2, 1, 0, 2, 3, 1)  // -X
                    CUBE_FACE(6, 4, 5, 6, 5, 7)  // +X
                    CUBE_FACE(0, 1, 5, 0, 5, 4)  // -Y
                    CUBE_FACE(2, 6, 7, 2, 7, 3)  // +Y
                    CUBE_FACE(0, 4, 6, 0, 6, 2)  // -Z
                    CUBE_FACE(1, 3, 7, 1, 7, 5)  // +Z
            };

            uint32_t cubeIndices[36] = {
                    0, 1, 2, 3, 4, 5,        // -X
                    6, 7, 8, 9, 10, 11,      // +X
                    12, 13, 14, 15, 16, 17,  // -Y
                    18, 19, 20, 21, 22, 23,  // +Y
                    24, 25, 26, 27, 28, 29,  // -Z
                    30, 31, 32, 33, 34, 35,  // +Z
            };

            m_vertexBuffer = CreateBuffer({BufferCreateInfo::Type::VERTEX, sizeof(float) * 4, sizeof(cubeVertices), &cubeVertices});
            m_indexBuffer = CreateBuffer({BufferCreateInfo::Type::INDEX, sizeof(uint32_t), sizeof(cubeIndices), &cubeIndices});

            size_t numberOfCuboids = 2;
            m_uniformBuffer_Camera = CreateBuffer({BufferCreateInfo::Type::UNIFORM, 0, sizeof(CameraConstants) * numberOfCuboids, nullptr});
            m_uniformBuffer_Normals = CreateBuffer({BufferCreateInfo::Type::UNIFORM, 0, sizeof(normals), &normals});

            std::string vertexSource = ReadTextFile("shaders/VertexShader_GLES.glsl", androidApp->activity->assetManager);
            Log::Write(Log::Level::Warning, Fmt("###### vertexSource: %s",vertexSource.c_str()));
            m_vertexShader = CreateShader({ShaderCreateInfo::Type::VERTEX, vertexSource.data(), vertexSource.size()});
            std::string fragmentSource = ReadTextFile("shaders/PixelShader_GLES.glsl", androidApp->activity->assetManager);
            Log::Write(Log::Level::Warning, Fmt("###### fragmentSource: %s",fragmentSource.c_str()));
            m_fragmentShader = CreateShader({ShaderCreateInfo::Type::FRAGMENT, fragmentSource.data(), fragmentSource.size()});
        }

        void DestroyResources() {

        }

        void DestroyDebugMessenger() {
            if (m_debugUtilsMessenger != XR_NULL_HANDLE) {
                DestroyOpenXRDebugUtilsMessenger(m_xrInstance, m_debugUtilsMessenger);
            }
        }

        void DestroyReferenceSpace() {
            Log::Write(Log::Level::Warning,"###### Destroy Reference Space");
//            OPENXR_CHECK(xrDestroySpace(ref), "Failed to destroy Reference Space.")
        }

        void DestroySession() {
            Log::Write(Log::Level::Warning,"###### Destroy Session");
            OPENXR_CHECK(xrDestroySession(m_session), "Failed to destroy Session.")
        }

        void DestroySwapchains() {
            Log::Write(Log::Level::Warning,"###### Destroy SwapChains");
            for(size_t i = 0; i < m_viewConfigurationViews.size(); i++){
                SwapchainInfo &colorSwapchainInfo = m_colorSwapchainInfos[i];
                SwapchainInfo *depthSwapchainInfo = &m_depthSwapchainInfos[i];

                for(void *&imageView : colorSwapchainInfo.imageViews){
                    DestroyImageView(imageView);
                }

                for(void *&imageView : depthSwapchainInfo->imageViews){
                    DestroyImageView(imageView);
                }

                FreeSwapchainImageData(colorSwapchainInfo.swapchain);
                FreeSwapchainImageData(depthSwapchainInfo->swapchain);

                OPENXR_CHECK(xrDestroySwapchain(colorSwapchainInfo.swapchain), "Failed to destroy Color Swapchain.")
                OPENXR_CHECK(xrDestroySwapchain(depthSwapchainInfo->swapchain), "Failed to destroy Depth Swapchain.")
            }
        }

        void GetInstanceProperties() {
            XrInstanceProperties instanceProperties{XR_TYPE_INSTANCE_PROPERTIES};
            OPENXR_CHECK(xrGetInstanceProperties(m_xrInstance, &instanceProperties), "Failed to get Instance Properties.")
            Log::Write(Log::Level::Info,Fmt("###### Instance Name: %s", instanceProperties.runtimeName));
        }

        void GetSystemID() {
            XrSystemGetInfo systemGetInfo{XR_TYPE_SYSTEM_GET_INFO};
            systemGetInfo.formFactor = m_formFactor;
            OPENXR_CHECK(xrGetSystem(m_xrInstance, &systemGetInfo, &m_systemID),"Failed to get SystemID.")

            OPENXR_CHECK(xrGetSystemProperties(m_xrInstance, m_systemID, &m_systemProperties), "Failed to get System Properties.")
        }

        void DestroyInstance() {
            OPENXR_CHECK(xrDestroyInstance(m_xrInstance), "Failed to destroy Instance.")
        }

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
                if (ALooper_pollAll(timeoutMilliseconds, nullptr, &events, (void **) &source) >=
                    0) {
                    if (source != nullptr) {
                        source->process(androidApp, source);
                    }
                } else {
                    break;
                }
            }
            // XR_DOCS_TAG_END_Android_System_Functionality2
        }

        void PollEvents(){
            XrEventDataBuffer eventData{XR_TYPE_EVENT_DATA_BUFFER};

            auto XrPollEvents = [&]()->bool {
                eventData = {XR_TYPE_EVENT_DATA_BUFFER};
                return xrPollEvent(m_xrInstance, &eventData) == XR_SUCCESS;
            };

            while (XrPollEvents()) {
                Log::Write(Log::Level::Info,Fmt("###### XrPollEvents: %d", eventData.type));
                switch (eventData.type) {
                    case XR_TYPE_EVENT_DATA_EVENTS_LOST: {
                        XrEventDataEventsLost *eventLost = reinterpret_cast<XrEventDataEventsLost *>(&eventData);
                        Log::Write(Log::Level::Info,Fmt("Events Lost: %d", eventLost->lostEventCount));
                        break;
                    }
                    case XR_TYPE_EVENT_DATA_INSTANCE_LOSS_PENDING: {
                        XrEventDataInstanceLossPending *instanceLossPending = reinterpret_cast<XrEventDataInstanceLossPending *>(&eventData);
                        Log::Write(Log::Level::Info,Fmt("Instance Loss Pending: %d", instanceLossPending->lossTime));
                        m_sessionRunning = false;
                        m_applicationRunning = false;
                        break;
                    }
                    case XR_TYPE_EVENT_DATA_INTERACTION_PROFILE_CHANGED: {
                        XrEventDataInteractionProfileChanged *interactionProfileChanged = reinterpret_cast<XrEventDataInteractionProfileChanged *>(&eventData);
                        Log::Write(Log::Level::Info,Fmt("Interaction Profile Changed: %d", interactionProfileChanged->session));
                        if(interactionProfileChanged->session !=m_session){
                            Log::Write(Log::Level::Info,"XrEventDataInteractionProfileChanged for unknown Session");
                            break;
                        }
                        break;
                    }
                    case XR_TYPE_EVENT_DATA_REFERENCE_SPACE_CHANGE_PENDING:{
                        XrEventDataReferenceSpaceChangePending *referenceSpaceChangePending = reinterpret_cast<XrEventDataReferenceSpaceChangePending *>(&eventData);
                        Log::Write(Log::Level::Info,Fmt("Reference Space Change Pending: %d", referenceSpaceChangePending->session));
                        if(referenceSpaceChangePending->session !=m_session){
                            Log::Write(Log::Level::Info,"XrEventDataReferenceSpaceChangePending for unknown Session");
                            break;
                        }
                    }
                    case XR_TYPE_EVENT_DATA_SESSION_STATE_CHANGED:{
                        XrEventDataSessionStateChanged *sessionStateChanged = reinterpret_cast<XrEventDataSessionStateChanged *>(&eventData);
                        Log::Write(Log::Level::Info,Fmt("Session State Changed: %d", sessionStateChanged->state));

                        m_sessionState = sessionStateChanged->state;
                        if(sessionStateChanged->session !=m_session){
                            Log::Write(Log::Level::Info,"XrEventDataSessionStateChanged for unknown Session");
                            break;
                        }

                        if(sessionStateChanged->state == XR_SESSION_STATE_READY){
                            // read, begin XrSession
                            XrSessionBeginInfo sessionBeginInfo{XR_TYPE_SESSION_BEGIN_INFO};
                            // type
                            sessionBeginInfo.primaryViewConfigurationType = XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO;
//                            sessionBeginInfo.primaryViewConfigurationType = XR_VIEW_CONFIGURATION_TYPE_PRIMARY_MONO;
                            OPENXR_CHECK(xrBeginSession(m_session, &sessionBeginInfo), "Failed to begin Session.")
                            m_sessionRunning = true;
                        }

                        if(sessionStateChanged->state == XR_SESSION_STATE_STOPPING){
                            OPENXR_CHECK(xrEndSession(m_session), "Failed to end Session.")
                            m_sessionRunning = false;
                        }

                        if(sessionStateChanged->state == XR_SESSION_STATE_EXITING){
                            m_sessionRunning = false;
                            m_applicationRunning = false;
                            Log::Write(Log::Level::Info,"XrEventDataSessionStateChanged for EXITING");
                        }

                        if(sessionStateChanged->state == XR_SESSION_STATE_LOSS_PENDING){
                            m_sessionRunning = false;
                            Log::Write(Log::Level::Info,"XrEventDataSessionStateChanged for LOSS_PENDING");
                        }
                        break;
                    }
                    default:
                        break;
                }
            }
        }

        XrSwapchainImageBaseHeader* AllocateSwapchainImageData(XrSwapchain swapchain, SwapchainType type, uint32_t count) {
            swapchainImagesMap[swapchain].first = type;
            swapchainImagesMap[swapchain].second.resize(count,
                                                        {XR_TYPE_SWAPCHAIN_IMAGE_OPENGL_ES_KHR});
            return reinterpret_cast<XrSwapchainImageBaseHeader *>(swapchainImagesMap[swapchain].second.data());
        }

        void* GetSwapchainImage(XrSwapchain swapchain, uint32_t index)  {
            return (void*)(uint64_t)swapchainImagesMap[swapchain].second[index].image;
        }

        void* CreateImageView(const ImageViewCreateInfo &imageViewCI) {
            GLuint framebuffer = 0;
            glGenFramebuffers(1, &framebuffer);

            GLenum attachment = imageViewCI.aspect == ImageViewCreateInfo::Aspect::COLOR_BIT ? GL_COLOR_ATTACHMENT0 : GL_DEPTH_ATTACHMENT;

            glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
            if (imageViewCI.view == ImageViewCreateInfo::View::TYPE_2D_ARRAY) {
                glFramebufferTextureMultiviewOVR(GL_DRAW_FRAMEBUFFER, attachment, (GLuint)(uint64_t)imageViewCI.image, imageViewCI.baseMipLevel, imageViewCI.baseArrayLayer, imageViewCI.layerCount);
            } else if (imageViewCI.view == ImageViewCreateInfo::View::TYPE_2D) {
                glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, attachment, GL_TEXTURE_2D, (GLuint)(uint64_t)imageViewCI.image, imageViewCI.baseMipLevel);
            } else {
                DEBUG_BREAK;
                std::cout << "ERROR: OPENGL: Unknown ImageView View type." << std::endl;
            }

            GLenum result = glCheckFramebufferStatus(GL_DRAW_FRAMEBUFFER);
            if (result != GL_FRAMEBUFFER_COMPLETE) {
                DEBUG_BREAK;
                std::cout << "ERROR: OPENGL: Framebuffer is not complete" << std::endl;
            }
            glBindFramebuffer(GL_FRAMEBUFFER, 0);

            imageViews[framebuffer] = imageViewCI;
            return (void *)(uint64_t)framebuffer;
        }

        void DestroyImageView(void *&imageView) {
            GLuint framebuffer = (GLuint)(uint64_t)imageView;
            imageViews.erase(framebuffer);
            glDeleteFramebuffers(1, &framebuffer);
            imageView = nullptr;
        }

        void FreeSwapchainImageData(XrSwapchain swapchain) {
            swapchainImagesMap[swapchain].second.clear();
            swapchainImagesMap.erase(swapchain);
        }

        void *CreateBuffer(const BufferCreateInfo &bufferCI) {
            GLuint buffer = 0;
            glGenBuffers(1, &buffer);

            GLenum target = 0;
            if (bufferCI.type == BufferCreateInfo::Type::VERTEX) {
                target = GL_ARRAY_BUFFER;
            } else if (bufferCI.type == BufferCreateInfo::Type::INDEX) {
                target = GL_ELEMENT_ARRAY_BUFFER;
            } else if (bufferCI.type == BufferCreateInfo::Type::UNIFORM) {
                target = GL_UNIFORM_BUFFER;
            } else {
                DEBUG_BREAK;
                std::cout << "ERROR: OPENGL: Unknown Buffer Type." << std::endl;
            }

            glBindBuffer(target, buffer);
            glBufferData(target, (GLsizeiptr)bufferCI.size, bufferCI.data, GL_STATIC_DRAW);
            glBindBuffer(target, 0);

            buffers[buffer] = bufferCI;
            return (void *)(uint64_t)buffer;
        }

    private:
        XrInstance m_xrInstance = {};

        std::vector<const char *> m_activeInstanceExtensions = {};
        std::vector<const char *> m_activeAPILayers = {};
        std::vector<std::string> m_apiLayers = {};
        std::vector<std::string> m_instanceExtensions = {};

        XrDebugUtilsMessengerEXT m_debugUtilsMessenger = XR_NULL_HANDLE;

        XrFormFactor m_formFactor = XR_FORM_FACTOR_HEAD_MOUNTED_DISPLAY;
        XrSystemId m_systemID = {};
        XrSystemProperties m_systemProperties = {XR_TYPE_SYSTEM_PROPERTIES};

        // session
        XrSession m_session = XR_NULL_HANDLE;
        XrSessionState m_sessionState = XR_SESSION_STATE_UNKNOWN;

        XrGraphicsBindingOpenGLESAndroidKHR m_graphicsBinding{XR_TYPE_GRAPHICS_BINDING_OPENGL_ES_ANDROID_KHR};
        ksGpuWindow window{};

        GLint m_contextApiMajorVersion{0};
        GLuint m_program{0};
        GLuint m_swapchainFramebuffer{0};

        GLint m_modelViewProjectionUniformLocation{0};
        GLint m_vertexAttribCoords{0};
        GLint m_vertexAttribColor{0};
        GLuint m_vao{0};
        GLuint m_cubeVertexBuffer{0};
        GLuint m_cubeIndexBuffer{0};

        bool m_applicationRunning = true;
        bool m_sessionRunning = false;

        // swapchain
        std::vector<XrViewConfigurationType> m_applicationViewConfigurations = {
                XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO,
                XR_VIEW_CONFIGURATION_TYPE_PRIMARY_MONO};
        std::vector<XrViewConfigurationType> m_viewConfigurations = {};
        XrViewConfigurationType m_viewConfigurationType = XR_VIEW_CONFIGURATION_TYPE_MAX_ENUM;
        std::vector<XrViewConfigurationView> m_viewConfigurationViews;
        XrViewConfigurationType m_viewConfiguration = XR_VIEW_CONFIGURATION_TYPE_MAX_ENUM;

        std::unordered_map<XrSwapchain, std::pair<SwapchainType, std::vector<XrSwapchainImageOpenGLESKHR>>> swapchainImagesMap{};
        std::unordered_map<GLuint, ImageViewCreateInfo> imageViews{};
        std::unordered_map<GLuint, BufferCreateInfo> buffers{};

        struct SwapchainInfo{
            XrSwapchain swapchain = XR_NULL_HANDLE;
            int64_t swapchainFormat = 0;
            std::vector<void *> imageViews;
        };

        struct CameraConstants {
            XrMatrix4x4f viewProj;
            XrMatrix4x4f modelViewProj;
            XrMatrix4x4f model;
            XrVector4f color;
            XrVector4f pad1;
            XrVector4f pad2;
            XrVector4f pad3;
        };
        CameraConstants cameraConstants;
        XrVector4f normals[6] = {
                {1.00f, 0.00f, 0.00f, 0},
                {-1.00f, 0.00f, 0.00f, 0},
                {0.00f, 1.00f, 0.00f, 0},
                {0.00f, -1.00f, 0.00f, 0},
                {0.00f, 0.00f, 1.00f, 0},
                {0.00f, 0.0f, -1.00f, 0}};

        std::vector<SwapchainInfo> m_colorSwapchainInfos = {};
        std::vector<SwapchainInfo> m_depthSwapchainInfos = {};

        XrSpace m_localSpace = XR_NULL_HANDLE;

        std::vector<XrEnvironmentBlendMode> m_applicationEnvironmentBlendModes = {
                XR_ENVIRONMENT_BLEND_MODE_OPAQUE,
                XR_ENVIRONMENT_BLEND_MODE_ADDITIVE};
        std::vector<XrEnvironmentBlendMode> m_environmentBlendModes = {};
        XrEnvironmentBlendMode m_environmentBlendMode = XR_ENVIRONMENT_BLEND_MODE_MAX_ENUM;

        // render
        GLuint setFramebuffer = 0;
        GLuint setPipeline = 0;
        GLuint vertexArray = 0;
        GLuint setIndexBuffer = 0;

        float m_viewHeightM = 1.5f;
        // Vertex and index buffers: geometry for our cuboids.
        void *m_vertexBuffer = nullptr;
        void *m_indexBuffer = nullptr;
        // Camera values constant buffer for the shaders.
        void *m_uniformBuffer_Camera = nullptr;
        // The normals are stored in a uniform buffer to simplify our vertex geometry.
        void *m_uniformBuffer_Normals = nullptr;

        // We use only two shaders in this app.
        void *m_vertexShader = nullptr, *m_fragmentShader = nullptr;

        // The pipeline is a graphics-API specific state object.
        void *m_pipeline = nullptr;
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



        OPENXR_CHECK(xrGetInstanceProcAddr(XR_NULL_HANDLE, "xrInitializeLoaderKHR",
                                           (PFN_xrVoidFunction *) &initializeLoader),
                     "Failed to get InstanceProcAddr for xrInitializeLoaderKHR.")

        if (!initializeLoader) {
            return;
        }

        //1. init loader
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
