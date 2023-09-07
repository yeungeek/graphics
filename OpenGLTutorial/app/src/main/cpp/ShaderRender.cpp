//
// Created by jian.yang on 2023/8/6.
//

#include "util/AndroidDebug.h"
#include <jni.h>
#include "sample/ShaderContext.h"

#define NATIVE_CLASS_NAME "com/yeungeek/opengltutorial/renderer/ShaderNativeRender"
#define NELEM(m) (sizeof(m) / sizeof((m)[0]))

#ifdef __cplusplus
extern "C" {
#endif
ShaderContext *context;

JNIEXPORT void JNICALL native_Init(JNIEnv *env, jobject thiz, jint id) {
    LOGD("###### native Init sample id:%d", id);
    context = ShaderContext::GetInstance(id);
}

JNIEXPORT void JNICALL native_UnInit(JNIEnv *env, jobject thiz) {
    LOGD("###### native UnInit");
    if (context)
        context->Destroy();
}

JNIEXPORT void JNICALL native_OnSurfaceCreated(JNIEnv *env, jobject thiz) {
    LOGD("###### native OnSurfaceCreated");
    context->OnSurfaceCreated();
}

JNIEXPORT void JNICALL native_OnSurfaceChanged(JNIEnv *env,
                                               jobject thiz,
                                               jint width,
                                               jint height) {
    LOGD("###### native OnSurfaceChanged");
    context->OnSurfaceChanged(width, height);
}

JNIEXPORT void JNICALL native_OnDrawFrame(JNIEnv *env, jobject thiz) {
    context->OnDrawFrame();
}

#ifdef __cplusplus
}
#endif

static int
RegisterNatives(JNIEnv *env, const char *clazzName, JNINativeMethod *method, int methodNum) {
    LOGI("###### register native method");
    jclass clazz = env->FindClass(clazzName);
    if (NULL == clazz) {
        LOGE("###### register native method error, class is null");
        return JNI_FALSE;
    }

    if (env->RegisterNatives(clazz, method, methodNum) < 0) {
        LOGE("###### register native method error");
        return JNI_FALSE;
    }

    return JNI_TRUE;
}

static void UnregisterNatives(JNIEnv *env, const char *clazzName) {
    LOGI("###### unregister native method");
    jclass clazz = env->FindClass(clazzName);
    if (NULL == clazz) {
        LOGE("###### unregister native method error, class is null");
        return;
    }

    env->UnregisterNatives(clazz);
}

static JNINativeMethod g_RenderMethods[] = {
        {"native_Init",             "(I)V",  (void *) (native_Init)},
        {"native_UnInit",           "()V",   (void *) (native_UnInit)},
        {"native_OnSurfaceCreated", "()V",   (void *) (native_OnSurfaceCreated)},
        {"native_OnSurfaceChanged", "(II)V", (void *) (native_OnSurfaceChanged)},
        {"native_OnDrawFrame",      "()V",   (void *) (native_OnDrawFrame)},
};

extern "C" jint JNI_OnLoad(JavaVM *vm, void *reserved) {
    LOGI("###### JNI OnLoad");
    jint result = JNI_ERR;
    JNIEnv *env = NULL;
    if (vm->GetEnv((void **) (&env), JNI_VERSION_1_6) != JNI_OK) {
        return result;
    }

    result = RegisterNatives(env, NATIVE_CLASS_NAME, g_RenderMethods, NELEM(g_RenderMethods));

    if (result != JNI_TRUE) {
        return JNI_ERR;
    }

    return JNI_VERSION_1_6;
}

extern "C" void JNI_OnUnload(JavaVM *vm, void *reserved) {
    LOGI("###### JNI OnUnLoad");

    JNIEnv *env = NULL;
    if (vm->GetEnv((void **) (&env), JNI_VERSION_1_6) != JNI_OK) {
        return;
    }

    UnregisterNatives(env, NATIVE_CLASS_NAME);
}