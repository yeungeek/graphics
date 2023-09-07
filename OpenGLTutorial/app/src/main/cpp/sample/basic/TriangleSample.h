//
// Created by jian.yang on 2023/8/11.
//

#ifndef OPENGLTUTORIAL_TRIANGLESAMPLE_H
#define OPENGLTUTORIAL_TRIANGLESAMPLE_H

#include "../BaseSample.h"

class TriangleSample : public BaseSample {
public:
    TriangleSample();
    virtual ~TriangleSample();
    virtual void OnCreate();
    virtual void OnDraw(int width, int height);
    virtual void OnDestroy();
};

#endif //OPENGLTUTORIAL_TRIANGLESAMPLE_H
