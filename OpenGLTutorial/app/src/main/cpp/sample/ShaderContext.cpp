//
// Created by jian.yang on 2023/8/8.
//

#include "ShaderContext.h"
#include "../util/AndroidDebug.h"

ShaderContext *ShaderContext::mContext = nullptr;

ShaderContext::ShaderContext() {
    LOGD("###### ShaderContext Constructor");
}

ShaderContext::ShaderContext(int id) {
    LOGD("###### ShaderContext Constructor id: %d", id);

}

ShaderContext::~ShaderContext() {
    LOGD("###### ShaderContext Destructor");
    if (mBaseSample) {
        delete mBaseSample;
        mBaseSample = nullptr;
    }
}

ShaderContext *ShaderContext::GetInstance(int id) {
    if (mContext == nullptr) {
        LOGD("###### ShaderContext Create");
        mContext = new ShaderContext(id);
    }

    return mContext;
}

void ShaderContext::Destroy() {
    LOGD("###### ShaderContext Destroy");
    if (mContext) {
        delete mContext;
        mContext = nullptr;
    }
}

void ShaderContext::OnSurfaceCreated() {
    LOGD("###### ShaderContext OnSurfaceCreated");
    glClearColor(1.0f, 1.0f, 1.0f, 1.0f);

}

void ShaderContext::OnSurfaceChanged(int width, int height) {
    LOGD("###### ShaderContext OnSurfaceChanged,w=%d,h=%d", width, height);
    glViewport(0, 0, width, height);
}

void ShaderContext::OnDrawFrame() {
    glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
}
