//
// Created by jian.yang on 2023/8/10.
//
#include "stdint.h"
#include <GLES3/gl3.h>

#ifndef OPENGLTUTORIAL_BASESAMPLE_H
#define OPENGLTUTORIAL_BASESAMPLE_H

#define SAMPLE_TYPE 1000
#define SAMPLE_TYPE_TRIANGLE SAMPLE_TYPE + 1
#define SAMPLE_TYPE_TRIANGLE_1 SAMPLE_TYPE + 2

class BaseSample {
public:
    BaseSample() {
        mProgram = 0;
        mVertexShaderId = 0;
        mFragmentShaderId = 0;
    }

    virtual ~BaseSample() {}

    virtual void OnCreate() = 0;

    virtual void OnDraw(int width, int height) = 0;

    virtual void OnDestroy() = 0;

protected:
    GLuint mProgram;
    GLuint mVertexShaderId;
    GLuint mFragmentShaderId;
};

#endif //OPENGLTUTORIAL_BASESAMPLE_H
