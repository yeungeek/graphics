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

struct engine {
    struct android_app *app;

    // sensor
    ASensorManager *sensorManager;
    ASensor *accelerometer;
    ASensorEventQueue *sensorEventQueue;

    // animation
    int animation;

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

}

