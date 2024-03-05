//
// Created by jian.yang on 2024/3/1.
//
#include <memory>
#include <cstdlib>
#include <cstring>
#include <jni.h>
#include <cerrno>

#include <EGL/egl.h>
#include <GLES3/gl3.h>

#include <android/log.h>
#include <android/sensor.h>

#include <android_native_app_glue.h>
#include <malloc.h>

#define LOGI(...) \
((void)__android_log_print( ANDROID_LOG_INFO, "native", __VA_ARGS__ ))

#define LOGW(...) \
((void)__android_log_print( ANDROID_LOG_WARN, "native", __VA_ARGS__ ))

struct saved_state {
    float angle;
    int32_t x;
    int32_t y;
};

struct engine {
    struct android_app *app;

    // sensor
    ASensorManager *sensorManager;
    const ASensor *accelerometer;
    ASensorEventQueue *sensorEventQueue;

    // animation
    int animating;

    // Display
    EGLDisplay display;
    EGLSurface surface;
    EGLContext context;
    int32_t width, height;

    struct saved_state state;
};


static int engine_init_display(struct engine *engine) {
    LOGI("engine init display");
    // init config
    const EGLint attribs[] = {
            EGL_SURFACE_TYPE, EGL_WINDOW_BIT,   // window type
            EGL_BLUE_SIZE, 8,
            EGL_GREEN_SIZE, 8,
            EGL_RED_SIZE, 8,
            EGL_NONE
    };

    EGLint w;
    EGLint h;
    EGLint dummy;
    EGLint format;
    EGLint numConfigs;
    EGLConfig config;
    EGLSurface surface;
    EGLContext context;

    // EGL display
    EGLDisplay display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    eglInitialize(display, 0, 0);
    eglChooseConfig(display, attribs, &config, 1, &numConfigs);
    // EGL Attrib
    eglGetConfigAttrib(display, config, EGL_NATIVE_VISUAL_ID, &format);

    ANativeWindow_setBuffersGeometry(engine->app->window, 0, 0, format);

    surface = eglCreateWindowSurface(display, config, engine->app->window, NULL);

    context = eglCreateContext(display, config, NULL, NULL);

    if (EGL_FALSE == eglMakeCurrent(display, surface, surface, context)) {
        LOGW("eglMakeCurrent failed");
        return -1;
    }

    eglQuerySurface(display, surface, EGL_WIDTH, &w);
    eglQuerySurface(display, surface, EGL_HEIGHT, &h);

    LOGI("###### eglCreateWindowSurface w=%d, h=%d", w, h);

    engine->display = display;
    engine->context = context;
    engine->surface = surface;
    engine->width = w;
    engine->height = h;

    engine->state.angle = 0.0f;

    glHint(GL_PROGRAM_BINARY_RETRIEVABLE_HINT, GL_FASTEST);
    glEnable(GL_CULL_FACE);
    glDisable(GL_DEPTH_TEST);

    return 0;
}

static void engine_draw_frame(struct engine *engine) {
    if (engine->display == nullptr) {
        return;
    }

    glClearColor(1, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    eglSwapBuffers(engine->display, engine->surface);
}

static void engine_term_display(struct engine *engine) {
    LOGI("engine term display");
    if (engine->display != EGL_NO_DISPLAY) {
        eglMakeCurrent(engine->display,
                       EGL_NO_SURFACE,
                       EGL_NO_SURFACE,
                       EGL_NO_CONTEXT);

        if (engine->context != EGL_NO_CONTEXT) {
            eglDestroyContext(engine->display, engine->context);
        }

        if (engine->surface != EGL_NO_SURFACE) {
            eglDestroySurface(engine->display, engine->surface);
        }

        eglTerminate(engine->display);
    }
}

static int32_t handle_input(struct android_app *app, AInputEvent *event) {
    LOGI("handle input");
    struct engine *engine = (struct engine *) app->userData;

    if (AInputEvent_getType(event) == AINPUT_EVENT_TYPE_MOTION) {
        engine->animating = 1;
        engine->state.x = AMotionEvent_getX(event, 0);
        engine->state.y = AMotionEvent_getY(event, 0);
        return 1;
    }

    return 0;
}


static void handle_cmd(struct android_app *app, int32_t cmd) {
    struct engine *engine = (struct engine *) app->userData;

    switch (cmd) {
        case APP_CMD_SAVE_STATE:
            LOGI("APP_CMD_SAVE_STATE");
            engine->app->savedState = malloc(sizeof(struct saved_state));

            *((struct saved_state *) (engine->app->savedState)) = engine->state;
            engine->app->savedStateSize = sizeof(struct saved_state);
            break;
        case APP_CMD_INIT_WINDOW:
            LOGI("APP_CMD_INIT_WINDOW");
            //window
            if (engine->app->window != NULL) {
                engine_init_display(engine);
                engine_draw_frame(engine);
            }
            break;
        case APP_CMD_TERM_WINDOW:
            LOGI("APP_CMD_TERM_WINDOW");
            // close window
            engine_term_display(engine);
            break;
        case APP_CMD_GAINED_FOCUS:
            LOGI("APP_CMD_GAINED_FOCUS");
            //focus
            if (engine->accelerometer != NULL) {
                ASensorEventQueue_enableSensor(engine->sensorEventQueue, engine->accelerometer);

                // event
                ASensorEventQueue_setEventRate(engine->sensorEventQueue, engine->accelerometer,
                                               (1000L / 60) * 1000);
                // 60
            }
            break;
        case APP_CMD_LOST_FOCUS:
            LOGI("APP_CMD_LOST_FOCUS");
            if (engine->accelerometer != NULL) {
                ASensorEventQueue_disableSensor(engine->sensorEventQueue, engine->accelerometer);
            }

            engine->animating = 0;
            engine_draw_frame(engine);
            break;
        case APP_CMD_STOP:
            LOGI("APP_CMD_STOP");

            break;
        case APP_CMD_DESTROY:
            break;
    }
}


void android_main(struct android_app *state) {
    //android_main
    LOGI("###### android_main, start activity...");
    struct engine engine;

    app_dummy();

    //reset
    memset(&engine, 0, sizeof(engine));

    //share
    state->userData = &engine;

    // handle cmd
    state->onAppCmd = handle_cmd;

    // handle input
    state->onInputEvent = handle_input;

    engine.app = state;
    //sensor
    engine.sensorManager = ASensorManager_getInstance();

    engine.accelerometer = ASensorManager_getDefaultSensor(engine.sensorManager,
                                                           ASENSOR_TYPE_ACCELEROMETER);
    engine.sensorEventQueue = ASensorManager_createEventQueue(engine.sensorManager, state->looper,
                                                              LOOPER_ID_USER, NULL, NULL);

    if (state->savedState != NULL) {
        // restore
        engine.state = *(struct saved_state *) state->savedState;
    }

    /**
     * loop
     */
    while (1) {
        int ident;
        int events;

        struct android_poll_source *source;
        // poll
        while ((ident = ALooper_pollAll(engine.animating ? 0 : -1,
                                        NULL,
                                        &events,
                                        (void **) &source)) >= 0) {
            if (source != NULL) {
                source->process(state, source);
            }

            if (ident == LOOPER_ID_USER) {
                if (engine.accelerometer != NULL) {
                    ASensorEvent event;
                    if (ASensorEventQueue_getEvents(engine.sensorEventQueue,
                                                    &event,
                                                    1) > 0) {
//                        LOGI("###### acceleration: x=%f, y=%f, z=%f",
//                             event.acceleration.x, event.acceleration.y, event.acceleration.z);
                    }
                }
            }

            if (state->destroyRequested != 0) {
                engine_term_display(&engine);
                break;
            }
        }

        if (engine.animating) {
            engine.state.angle += 0.01f;
            if (engine.state.angle > 1) {
                engine.state.angle = 0;
            }

            engine_draw_frame(&engine);
        }
    }
}

