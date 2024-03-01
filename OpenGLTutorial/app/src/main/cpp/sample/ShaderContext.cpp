//
// Created by jian.yang on 2023/8/8.
//

#include "ShaderContext.h"
#include "../util/AndroidDebug.h"
#include "basic/TriangleSample.h"

ShaderContext *ShaderContext::mContext = nullptr;

ShaderContext::ShaderContext() {
    LOGD("###### ShaderContext Constructor");

}

ShaderContext::ShaderContext(int id) {
    LOGD("###### ShaderContext Constructor id: %d", id);
    switch (id) {
        case SAMPLE_TYPE_TRIANGLE:
            mBaseSample = new TriangleSample();
            break;
        case SAMPLE_TYPE_TRIANGLE_1:
//            mBaseSample
            break;
        default:
            mBaseSample = new TriangleSample();
            break;
    }

}

ShaderContext::~ShaderContext() {
    LOGD("###### ShaderContext Destructor");
    if (nullptr != mBaseSample) {
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
    if (mBaseSample) {
        mBaseSample->OnDestroy();
    }
    if (mContext) {
        delete mContext;
        mContext = nullptr;
    }
}

void ShaderContext::OnSurfaceCreated() {
    LOGD("###### ShaderContext OnSurfaceCreated");
    glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
    if (mBaseSample) {
        mBaseSample->OnCreate();
    }
}

void ShaderContext::OnSurfaceChanged(int width, int height) {
    LOGD("###### ShaderContext OnSurfaceChanged,w=%d,h=%d", width, height);
    glViewport(0, 0, width, height);
    mWidth = width;
    mHeight = height;
}

void ShaderContext::OnDrawFrame() {
    if (mBaseSample) {
        mBaseSample->OnDraw(mWidth, mHeight);
    }
}
