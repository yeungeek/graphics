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

#include <common/gfxwrapper_opengl.h>

#include <android/log.h>
#include <android_native_app_glue.h>
#include <android/native_window.h>
#include <jni.h>
#include <sys/system_properties.h>

namespace Chapter2 {
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
            CreateSession();
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
            glDebugMessageCallback(
                    [](GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* message,
                       const void* userParam) {
                        Log::Write(Log::Level::Info, "###### GLES Debug: " + std::string(message, 0, length));
                    },
                    this);

            // Initialize the resources
            InitializeResources();
        }

        void InitializeResources() {
            Log::Write(Log::Level::Info,"###### InitializeResources Start");
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

            m_modelViewProjectionUniformLocation = glGetUniformLocation(m_program, "ModelViewProjection");

            m_vertexAttribCoords = glGetAttribLocation(m_program, "VertexPos");
            m_vertexAttribColor = glGetAttribLocation(m_program, "VertexColor");

            glGenBuffers(1, &m_cubeVertexBuffer);
            glBindBuffer(GL_ARRAY_BUFFER, m_cubeVertexBuffer);
            glBufferData(GL_ARRAY_BUFFER, sizeof(Geometry::c_cubeVertices), Geometry::c_cubeVertices, GL_STATIC_DRAW);

            glGenBuffers(1, &m_cubeIndexBuffer);
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_cubeIndexBuffer);
            glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(Geometry::c_cubeIndices), Geometry::c_cubeIndices, GL_STATIC_DRAW);

            glGenVertexArrays(1, &m_vao);
            glBindVertexArray(m_vao);
            glEnableVertexAttribArray(m_vertexAttribCoords);
            glEnableVertexAttribArray(m_vertexAttribColor);
            glBindBuffer(GL_ARRAY_BUFFER, m_cubeVertexBuffer);
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_cubeIndexBuffer);
            glVertexAttribPointer(m_vertexAttribCoords, 3, GL_FLOAT, GL_FALSE, sizeof(Geometry::Vertex), nullptr);
            glVertexAttribPointer(m_vertexAttribColor, 3, GL_FLOAT, GL_FALSE, sizeof(Geometry::Vertex),
                                  reinterpret_cast<const void*>(sizeof(XrVector3f)));
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

        void DestroyDebugMessenger() {
            if (m_debugUtilsMessenger != XR_NULL_HANDLE) {
                DestroyOpenXRDebugUtilsMessenger(m_xrInstance, m_debugUtilsMessenger);
            }
        }

        void DestroySession() {
            OPENXR_CHECK(xrDestroySession(m_session), "Failed to destroy Session.")
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
