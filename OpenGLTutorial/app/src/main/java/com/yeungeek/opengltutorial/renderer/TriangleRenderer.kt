package com.yeungeek.opengltutorial.renderer

import android.opengl.GLES30
import android.opengl.GLSurfaceView.Renderer
import java.nio.ByteBuffer
import java.nio.ByteOrder
import java.nio.FloatBuffer
import javax.microedition.khronos.egl.EGLConfig
import javax.microedition.khronos.opengles.GL10


open class TriangleRenderer : Renderer {
    private val vertexPoints = floatArrayOf(
        0.0f, 0.5f, 0.0f,
        -0.5f, -0.5f, 0.0f,
        0.5f, -0.5f, 0.0f
    )

    var vertexBuffer: FloatBuffer

    /**
     * vertex
     */
    private val vertexShader = """#version 300 es
                layout (location = 0) in vec4 vPosition;
                void main() {
                     gl_Position  = vPosition;
                }
                """

    /**
     * fragment
     */
    private val fragmentShader = """#version 300 es
                precision mediump float;
                out vec4 fragColor;
                void main() {
                     fragColor = vec4(1.0,0.0,0.0,1.0);
                }
                """

    init {
        vertexBuffer = ByteBuffer.allocateDirect(vertexPoints.size * 4)
            .order(ByteOrder.nativeOrder())
            .asFloatBuffer()

        vertexBuffer.put(vertexPoints)
        vertexBuffer.position(0)
    }

    private fun compileShader(type: Int, shaderCode: String): Int {
        //创建一个着色器
        val shaderId = GLES30.glCreateShader(type)
        return if (shaderId != 0) {
            //加载到着色器
            GLES30.glShaderSource(shaderId, shaderCode)
            //编译着色器
            GLES30.glCompileShader(shaderId)
            //检测状态
            val compileStatus = IntArray(1)
            GLES30.glGetShaderiv(shaderId, GLES30.GL_COMPILE_STATUS, compileStatus, 0)
            if (compileStatus[0] == 0) {
                val logInfo = GLES30.glGetShaderInfoLog(shaderId)
                System.err.println(logInfo)
                //创建失败
                GLES30.glDeleteShader(shaderId)
                return 0
            }
            shaderId
        } else {
            return 0
        }
    }

    private fun linkProgram(vertexShaderId: Int, fragmentShaderId: Int): Int {
        val programId = GLES30.glCreateProgram()
        return if (programId != 0) {
            //将顶点着色器加入到程序
            GLES30.glAttachShader(programId, vertexShaderId)
            //将片元着色器加入到程序中
            GLES30.glAttachShader(programId, fragmentShaderId)
            //链接着色器程序
            GLES30.glLinkProgram(programId)
            val linkStatus = IntArray(1)
            GLES30.glGetProgramiv(programId, GLES30.GL_LINK_STATUS, linkStatus, 0)
            if (linkStatus[0] == 0) {
                val logInfo = GLES30.glGetProgramInfoLog(programId)
                System.err.println(logInfo)
                GLES30.glDeleteProgram(programId)
                return 0
            }
            programId
        } else {
            return 0
        }
    }

    fun create() {
    }

    fun destroy() {
    }

    override fun onSurfaceCreated(gl10: GL10?, eglConfig: EGLConfig?) {
        GLES30.glClearColor(1f, 1f, 1f, 1f)
        val vertexShaderId = compileShader(GLES30.GL_VERTEX_SHADER, vertexShader)
        val fragmentShaderId = compileShader(GLES30.GL_FRAGMENT_SHADER, fragmentShader)
        GLES30.glUseProgram(linkProgram(vertexShaderId, fragmentShaderId))
    }

    override fun onSurfaceChanged(gl10: GL10?, width: Int, height: Int) {
        GLES30.glViewport(0, 0, width, height)
    }

    override fun onDrawFrame(gl10: GL10?) {
        GLES30.glClear(GLES30.GL_COLOR_BUFFER_BIT)
        //准备坐标数据
        GLES30.glVertexAttribPointer(0, 3, GLES30.GL_FLOAT, false, 0, vertexBuffer)
        //启用顶点的句柄
        GLES30.glEnableVertexAttribArray(0)
        //绘制三个点
//        GLES30.glDrawArrays(GLES30.GL_LINE_LOOP, 0, 3)

        //绘制直线
//        GLES30.glDrawArrays(GLES30.GL_LINE_STRIP, 0, 2)
//        GLES30.glLineWidth(10f)

        //绘制三角形
        GLES30.glDrawArrays(GLES30.GL_TRIANGLES, 0, 3)
        //禁止顶点数组的句柄
        GLES30.glDisableVertexAttribArray(0)
    }
}