//
// Created by jian.yang on 2023/8/6.
//

#ifndef OPENGLTUTORIAL_ANDROIDDEBUG_H
#define OPENGLTUTORIAL_ANDROIDDEBUG_H

#include "android/log.h"

#define LOG_TAG "ShaderRender"

#define LOGV(...) \
    __android_log_print(ANDROID_LOG_VERBOSE,LOG_TAG,__VA_ARGS__)
#define LOGD(...) \
  __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)
#define LOGI(...) \
  __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGW(...) \
  __android_log_print(ANDROID_LOG_WARN, LOG_TAG, __VA_ARGS__)
#define LOGE(...) \
  __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)
#define LOGF(...) \
  __android_log_print(ANDROID_LOG_FATAL, LOG_TAG, __VA_ARGS__)

#endif //OPENGLTUTORIAL_ANDROIDDEBUG_H
