//
// Created by jian.yang on 2023/8/21.
//

#include "GLUtils.h"
#include "AndroidDebug.h"

GLuint GLUtils::LoaderShader(GLenum shaderType, const char *source) {
    LOGD("###### Loader Shader");
    GLuint shader = 0;

    shader = glCreateShader(shaderType);
    if (shader) {
        glShaderSource(shader, 1, &source, NULL);
        glCompileShader(shader);

        GLint compiled = 0;
        glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
        if (!compiled) {
            GLint infoLen = 0;
            glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &infoLen);
            if (infoLen) {
                char *buf = (char *) malloc((size_t) infoLen);
                if (buf) {
                    glGetShaderInfoLog(shader, infoLen, NULL, buf);
                    LOGE("###### LoadShader compiled error %d, \n%s", shaderType, buf);
                    free(buf);
                }
                glDeleteShader(shader);
                shader = 0;
            }
        }
    }

    return shader;
}

GLuint
GLUtils::CreateProgram(const char *vertexShader, const char *fragmentShader, GLuint &vertexShaderId,
                       GLuint &fragmentShaderId) {
    GLuint program = 0;
    LOGI("###### Create Program Start");
    vertexShaderId = LoaderShader(GL_VERTEX_SHADER, vertexShader);
    if (!vertexShaderId) {
        return program;
    }
    fragmentShaderId = LoaderShader(GL_FRAGMENT_SHADER, fragmentShader);
    if (!fragmentShaderId) {
        return program;
    }

    program = glCreateProgram();
    LOGD("###### Create Program Result: %d", program);
    if (program) {
        glAttachShader(program, vertexShaderId);
        CheckGLError("glAttachShader");
        glAttachShader(program, fragmentShaderId);
        CheckGLError("glAttachShader");
        //link
        glLinkProgram(program);
        GLint linkResult = GL_FALSE;
        glGetProgramiv(program, GL_LINK_STATUS, &linkResult);

        //delete
        glDetachShader(program, vertexShaderId);
        glDeleteShader(vertexShaderId);
        vertexShaderId = 0;
        glDetachShader(program, fragmentShaderId);
        glDeleteShader(fragmentShaderId);
        fragmentShaderId = 0;

        LOGD("###### Link Program Result: %d", linkResult);
        if (linkResult != GL_TRUE) {
            GLint infoLen = 0;
            glGetProgramiv(program, GL_INFO_LOG_LENGTH, &infoLen);
            if (infoLen) {
                char *buf = (char *) malloc((size_t) infoLen);
                if (buf) {
                    glGetProgramInfoLog(program, infoLen, NULL, buf);
                    LOGE("Create Program Link Error: \n %s", buf);
                    free(buf);
                }
            }

            glDeleteProgram(program);
            program = 0;
        }
    }

    LOGI("###### Create Program Success: %d", program);
    return program;
}

void GLUtils::CheckGLError(const char *glPipline) {
    for (GLint error = glGetError(); error; error = glGetError()) {
        LOGE("###### Check GL Error: %s, glError: (0x%d)", glPipline, error);
    }
}



