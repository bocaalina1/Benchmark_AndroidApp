// file: com/example/myapplication/MyGLSurfaceView.kt

package com.example.myapplication

import android.content.Context
import android.opengl.GLSurfaceView
import android.util.AttributeSet

// Add '@JvmOverloads' to automatically generate the necessary constructors
class MyGLSurfaceView @JvmOverloads constructor(
    context: Context,
    attrs: AttributeSet? = null // Add the second argument
) : GLSurfaceView(context, attrs) {

    private val renderer: MyGLRenderer

    init {
        // This setup code stays exactly the same
        setEGLContextClientVersion(3)
        renderer = MyGLRenderer(context)
        setRenderer(renderer)
        renderMode = RENDERMODE_CONTINUOUSLY
    }
}