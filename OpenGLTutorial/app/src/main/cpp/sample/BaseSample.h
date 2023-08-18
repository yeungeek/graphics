//
// Created by jian.yang on 2023/8/10.
//
#include "stdint.h"

#ifndef OPENGLTUTORIAL_BASESAMPLE_H
#define OPENGLTUTORIAL_BASESAMPLE_H

#define SAMPLE_TYPE 1000
#define SAMPLE_TYPE_TRIANGLE SAMPLE_TYPE + 1

class BaseSample {
public:
    BaseSample() {

    }

    virtual ~BaseSample() {}

    virtual void OnCreate() = 0;

    virtual void OnDraw(int width, int height) = 0;

    virtual void OnDestroy() = 0;

protected:
    GLuint mProgram;

};

#endif //OPENGLTUTORIAL_BASESAMPLE_H
