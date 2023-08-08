//
// Created by jian.yang on 2023/8/7.
//

#include <GLES3/gl3.h>

#ifndef OPENGLTUTORIAL_SHADERCONTEXT_H
#define OPENGLTUTORIAL_SHADERCONTEXT_H

class ShaderContext{
    ShaderContext();
    ~ShaderContext();

public:

    static ShaderContext* GetInstance();
    static void Destroy();
    void OnSurfaceCreated();
    void OnSurfaceChanged(int width, int height);
    void OnDrawFrame();

private:
    static ShaderContext *sContext;
    int sWidth;
    int sHeight;
};

#endif //OPENGLTUTORIAL_SHADERCONTEXT_H
