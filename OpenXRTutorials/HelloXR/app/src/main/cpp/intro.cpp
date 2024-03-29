//
// Created by jian.yang on 2024/3/29.
//
#include <string.h>
#include <iostream>

#include "common/logger.h"
#include "common/common.h"

#include <android/log.h>
#include <android_native_app_glue.h>
#include <android/native_window.h>
#include <jni.h>
#include <sys/system_properties.h>

extern "C"
void android_main(struct android_app *app) {
    Log::Write(Log::Level::Info, "###### main");
    JNIEnv *Env;
    app->activity->vm->AttachCurrentThread(&Env, nullptr);
}