//
// Created by jian.yang on 2023/8/7.
//

#include <GLES3/gl3.h>
#include "BaseSample.h"

#ifndef OPENGLTUTORIAL_SHADERCONTEXT_H
#define OPENGLTUTORIAL_SHADERCONTEXT_H

class ShaderContext{
    ShaderContext();
    ShaderContext(int id);
    ~ShaderContext();

public:

    static ShaderContext* GetInstance(int id);
    void Destroy();
    void OnSurfaceCreated();
    void OnSurfaceChanged(int width, int height);
    void OnDrawFrame();

private:
    static ShaderContext *mContext;

    BaseSample *mBaseSample;
    int mWidth;
    int mHeight;
};

#endif //OPENGLTUTORIAL_SHADERCONTEXT_H
