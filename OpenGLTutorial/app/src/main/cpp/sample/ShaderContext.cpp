//
// Created by jian.yang on 2023/8/8.
//

#include "ShaderContext.h"
#include "../util/AndroidDebug.h"

ShaderContext *ShaderContext::sContext = nullptr;

ShaderContext::ShaderContext() {

}

ShaderContext::~ShaderContext() {

}

ShaderContext *ShaderContext::GetInstance() {
    if (sContext == nullptr) {
        sContext = new ShaderContext();
    }

    return sContext;
}

void ShaderContext::Destroy() {
    if (sContext) {
        delete sContext;
        sContext = nullptr;
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