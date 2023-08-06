package com.yeungeek.opengltutorial.renderer

import android.opengl.GLES10.GL_COLOR_BUFFER_BIT
import android.opengl.GLES10.GL_DEPTH_BUFFER_BIT
import android.opengl.GLES20
import android.opengl.GLES30
import android.opengl.GLSurfaceView.Renderer
import android.util.Log
import javax.microedition.khronos.egl.EGLConfig
import javax.microedition.khronos.opengles.GL10

open class ShaderRenderer : Renderer {
    init {
        Log.d("DEBUG", "##### Renderer Init")
    }

    override fun onSurfaceCreated(gl: GL10?, config: EGLConfig?) {
        Log.d("DEBUG", "###### onSurfaceCreated")
        GLES30.glClearColor(0f, 0f, 0f, 1f)

    }

    override fun onSurfaceChanged(gl: GL10?, width: Int, height: Int) {
        Log.d("DEBUG", "###### onSurfaceChanged")
        GLES20.glViewport(0, 0, width, height)
    }

    override fun onDrawFrame(gl: GL10?) {
        GLES30.glClear(GL_DEPTH_BUFFER_BIT or GL_COLOR_BUFFER_BIT)
    }
}