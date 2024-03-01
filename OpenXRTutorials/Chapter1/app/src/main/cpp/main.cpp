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
((void)__android_log_print( ANDROID_LOG_INFO, "native-activity", __VA_ARGS__ ))

#define LOGW(...) \
((void)__android_log_print( ANDROID_LOG_WARN, "native-activity", __VA_ARGS__ ))

struct engine {
    struct android_app *app;

    // sensor
    ASensorManager *sensorManager;

};

void android_main(struct android_app *state) {
    //android_main
    LOGI("android_main, start activity...");
    struct engine *engine;

    app_dummy();

    //reset
    memset(&engine, 0, sizeof(engine));

    //share
    state->userData = &engine;

    //
}
