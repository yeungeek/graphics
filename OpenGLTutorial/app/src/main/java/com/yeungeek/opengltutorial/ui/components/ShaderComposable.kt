package com.yeungeek.opengltutorial.ui.components

import android.opengl.GLSurfaceView.DEBUG_CHECK_GL_ERROR
import android.opengl.GLSurfaceView.DEBUG_LOG_GL_CALLS
import androidx.compose.runtime.Composable
import androidx.compose.runtime.DisposableEffect
import androidx.compose.ui.Modifier
import androidx.compose.ui.platform.LocalLifecycleOwner
import androidx.compose.ui.viewinterop.AndroidView
import androidx.lifecycle.Lifecycle
import androidx.lifecycle.LifecycleEventObserver
import com.yeungeek.opengltutorial.renderer.ShaderGLSurfaceView
import com.yeungeek.opengltutorial.renderer.ShaderRenderer
import com.yeungeek.opengltutorial.renderer.TriangleRenderer

@Composable
fun GLShader(
    renderer: ShaderRenderer,
    modifier: Modifier
) {

    val lifeCycleState = LocalLifecycleOwner.current.lifecycle

    DisposableEffect(key1 = lifeCycleState) {
        val observer = LifecycleEventObserver { _, event ->
            when (event) {
                Lifecycle.Event.ON_CREATE -> {
                    renderer.create()
                }

                Lifecycle.Event.ON_RESUME -> {

                }

                Lifecycle.Event.ON_PAUSE -> {

                }

                Lifecycle.Event.ON_DESTROY -> {
                    renderer.destroy()
                }

                else -> {
                }
            }
        }
        lifeCycleState.addObserver(observer)

        onDispose {
            lifeCycleState.removeObserver(observer)
        }
    }

    AndroidView(modifier = modifier, factory = { ShaderGLSurfaceView(it) }) { glSurfaceView ->
        glSurfaceView.debugFlags = DEBUG_CHECK_GL_ERROR or DEBUG_LOG_GL_CALLS
        glSurfaceView.setShaderRenderer(renderer)
    }
}