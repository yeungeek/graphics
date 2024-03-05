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

static void handle_cmd(struct android_app *app, int32_t cmd) {
    struct engine *engine = (struct engine *) app->userData;

    switch (cmd) {
        case APP_CMD_INIT_WINDOW:
            LOGI("APP_CMD_INIT_WINDOW");
            break;
        case APP_CMD_TERM_WINDOW:
            LOGI("APP_CMD_TERM_WINDOW");
            break;
        case APP_CMD_STOP:
            LOGI("APP_CMD_STOP");
            break;
        case APP_CMD_DESTROY:
            break;
    }
        case APP_CMD_INIT_WINDOW:
            LOGI("APP_CMD_INIT_WINDOW");
            break;
        case APP_CMD_TERM_WINDOW:
            LOGI("APP_CMD_TERM_WINDOW");
            break;
        case APP_CMD_STOP:
            LOGI("APP_CMD_STOP");
            break;
        case APP_CMD_DESTROY:
            break;
    }
}

static int32_t handle_input(struct android_app *app, AInputEvent *event) {
    struct engine *engine = (struct engine *) app->userData;

    if (AInputEvent_getType(event) == AINPUT_EVENT_TYPE_MOTION) {
        int32_t action = AMotionEvent_getAction(event);
        if (action == AMOTION_EVENT_ACTION_DOWN) {
            LOGI("AMOTION_EVENT_ACTION_DOWN");
        } else if (action == AMOTION_EVENT_ACTION_UP) {

        }
    }
}


static void engine_term_display(struct engine *engine) {
    if (engine->display != EGL_NO_DISPLAY) {
        EGLBoolean result;
        result = eglMakeCurrent(engine->display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
        LOGI("eglMakeCurrent result: %d", result);
        result = eglTerminate(engine->display);
        LOGI("eglTerminate result: %d", result);
    }
}

static void engine_draw_frame(struct engine *engine) {

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
                        LOGI("###### acceleration: x=%f, y=%f, z=%f",
                             event.acceleration.x, event.acceleration.y, event.acceleration.z);
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

