package com.yeungeek.opengltutorial.renderer

import android.opengl.GLSurfaceView.Renderer
import android.util.Log
import com.yeungeek.opengltutorial.data.Sample
import javax.microedition.khronos.egl.EGLConfig
import javax.microedition.khronos.opengles.GL10

open class ShaderRenderer(sample: Sample) : Renderer {
    private val nativeRender: ShaderNativeRender
    private val mSample:Sample
    init {
        Log.d("DEBUG", "##### Renderer Init")
        nativeRender = ShaderNativeRender()
        mSample = sample
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
        nativeRender.native_Init(mSample.id)
    }

    fun destroy() {
        nativeRender.native_UnInit()
    }
}