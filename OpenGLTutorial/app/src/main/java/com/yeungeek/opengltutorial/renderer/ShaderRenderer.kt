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
    private val nativeRender: ShaderNativeRender

    init {
        Log.d("DEBUG", "##### Renderer Init")
        nativeRender = ShaderNativeRender()
    }

    override fun onSurfaceCreated(gl: GL10?, config: EGLConfig?) {
        nativeRender.native_OnSurfaceCreated()
    }

    override fun onSurfaceChanged(gl: GL10?, width: Int, height: Int) {
        nativeRender.native_OnSurfaceChanged(width,height)
    }

    override fun onDrawFrame(gl: GL10?) {
        nativeRender.native_OnDrawFrame()
    }

    fun create() {
        nativeRender.native_Init()
    }

    fun destroy() {
        nativeRender.native_UnInit()
    }
}