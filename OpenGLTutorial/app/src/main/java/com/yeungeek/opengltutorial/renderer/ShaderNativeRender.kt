package com.yeungeek.opengltutorial.renderer

class ShaderNativeRender() {
    companion object {
        init {
            System.loadLibrary("shader-render")
        }
    }

    external fun native_Init(id:Int)

    external fun native_UnInit()

    external fun native_OnSurfaceCreated()

    external fun native_OnSurfaceChanged(width: Int, height: Int)

    external fun native_OnDrawFrame()
}