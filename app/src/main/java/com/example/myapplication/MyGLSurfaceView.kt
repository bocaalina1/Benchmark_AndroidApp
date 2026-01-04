package com.example.myapplication

import android.content.Context
import android.opengl.GLSurfaceView
import android.util.AttributeSet
import android.util.Log

class MyGLSurfaceView @JvmOverloads constructor(
    context: Context,
    attrs: AttributeSet? = null
) : GLSurfaceView(context, attrs) {

    private val renderer: MyGLRenderer

    init {
        Log.i("MyGLSurfaceView", "Initializing GLSurfaceView")

        // Set OpenGL ES 3.0
        setEGLContextClientVersion(3)

        // Configure EGL for better compatibility
        setEGLConfigChooser(8, 8, 8, 8, 16, 8)

        // Create and set renderer
        renderer = MyGLRenderer(context)
        setRenderer(renderer)

        // Use continuous rendering for animations
        renderMode = RENDERMODE_CONTINUOUSLY

        // Preserve context on pause (optional but recommended)
        preserveEGLContextOnPause = true

        Log.i("MyGLSurfaceView", "GLSurfaceView initialized successfully")
    }
}