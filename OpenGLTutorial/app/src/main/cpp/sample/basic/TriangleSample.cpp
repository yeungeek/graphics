//
// Created by jian.yang on 2023/8/21.
//
#include "TriangleSample.h"
#include "../../util/GLUtils.h"
#include "../../util/AndroidDebug.h"

GLfloat mVertices[9] = {
        0.0f, 0.5f, 0.0f,
        -0.5f, -0.5f, 0.0f,
        0.5f, -0.5f, 0.0f
};

TriangleSample::TriangleSample() {
    LOGD("###### TriangleSample init");
}

TriangleSample::~TriangleSample() {

}

void TriangleSample::OnCreate() {
    if (mProgram != 0) {
        return;
    }

    char vertexShader[] =
            "#version 300 es                          \n"
            "layout(location = 0) in vec4 vPosition;  \n"
            "void main()                              \n"
            "{                                        \n"
            "   gl_Position = vPosition;              \n"
            "}                                        \n";

    char fragmentShader[] =
            "#version 300 es                              \n"
            "precision mediump float;                     \n"
            "out vec4 fragColor;                          \n"
            "void main()                                  \n"
            "{                                            \n"
            "   fragColor = vec4 ( 1.0, 0.0, 0.0, 1.0 );  \n"
            "}                                            \n";

    mProgram = GLUtils::CreateProgram(vertexShader, fragmentShader, mVertexShaderId,
                                      mFragmentShaderId);

    glUseProgram(mProgram);
    LOGD("###### Program init %d", mProgram);
}

void TriangleSample::OnDraw(int width, int height) {
    if (mProgram == 0) {
        return;
    }

    //clear
    glClear(GL_COLOR_BUFFER_BIT);

    //load vertex data
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, mVertices);
    glEnableVertexAttribArray(0);

    //draw
    glDrawArrays(GL_TRIANGLES, 0, 3);

    glDisableVertexAttribArray(0);

}

void TriangleSample::OnDestroy() {
    glUseProgram(GL_NONE);
    if (mProgram) {
        glDeleteProgram(mProgram);
        mProgram = GL_NONE;
    }
}