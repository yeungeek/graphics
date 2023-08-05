package com.yeungeek.opengltutorial.renderer

import android.content.Context
import android.opengl.GLSurfaceView
import android.util.AttributeSet

class ShaderGLSurfaceView @JvmOverloads constructor(
    context: Context,
    attrs: AttributeSet? = null
) : GLSurfaceView(context, attrs) {
    init {
        setEGLContextClientVersion(3)

        preserveEGLContextOnPause = true
    }

    private var hasSetRender = false

    fun setShaderRenderer(
        renderer: Renderer
    ) {
        if (hasSetRender.not()) {
            setRenderer(renderer)
        }

        hasSetRender = true
    }

    override fun onResume() {
        super.onResume()
    }

    override fun onPause() {
        super.onPause()
    }
}