package com.yeungeek.opengltutorial.ui.components

import android.opengl.GLSurfaceView
import android.opengl.GLSurfaceView.DEBUG_CHECK_GL_ERROR
import android.opengl.GLSurfaceView.DEBUG_LOG_GL_CALLS
import androidx.compose.runtime.Composable
import androidx.compose.runtime.remember
import androidx.compose.ui.Modifier
import androidx.compose.ui.viewinterop.AndroidView
import com.yeungeek.opengltutorial.renderer.ShaderGLSurfaceView
import com.yeungeek.opengltutorial.renderer.ShaderRenderer

@Composable
fun GLShader(
    renderer: ShaderRenderer,
    modifier: Modifier
) {

    AndroidView(modifier = modifier, factory = { ShaderGLSurfaceView(it) }) { glSurfaceView ->
        glSurfaceView.debugFlags = DEBUG_CHECK_GL_ERROR or DEBUG_LOG_GL_CALLS
        glSurfaceView.setShaderRenderer(renderer)
    }
}