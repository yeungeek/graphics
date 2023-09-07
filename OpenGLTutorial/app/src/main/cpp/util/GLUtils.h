//
// Created by jian.yang on 2023/8/21.
//

#ifndef OPENGLTUTORIAL_GLUTILS_H
#define OPENGLTUTORIAL_GLUTILS_H

#include <GLES3/gl3.h>
#include <glm.hpp>

class GLUtils {
public:
    static GLuint LoaderShader(GLenum shaderType, const char *source);

    static GLuint CreateProgram(const char *vertexShader, const char *fragmentShader,
                                GLuint &vertexShaderId, GLuint &fragmentShaderId);

    static void CheckGLError(const char *glPipline);
};

#endif //OPENGLTUTORIAL_GLUTILS_H
