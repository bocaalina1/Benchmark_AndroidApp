package com.example.myapplication

import android.content.Context
import android.content.res.AssetManager
import android.opengl.GLSurfaceView
import android.util.Log
import javax.microedition.khronos.egl.EGLConfig
import javax.microedition.khronos.opengles.GL10

class MyGLRenderer(private val context: Context) : GLSurfaceView.Renderer {

    companion object {
        private const val TAG = "MyGLRenderer"

        init {
            try {
                System.loadLibrary("myapplication")
                Log.i(TAG, "Native library loaded successfully")
            } catch (e: UnsatisfiedLinkError) {
                Log.e(TAG, "Failed to load native library: ${e.message}")
                throw e
            }
        }
    }

    private var isInitialized = false
    private var firstFrame = true

    // Native method declarations - these MUST match your C++ function names
    private external fun nativeSetAssetManager(assetManager: AssetManager)
    private external fun nativeOnSurfaceCreated()
    private external fun nativeOnSurfaceChanged(width: Int, height: Int)
    private external fun nativeOnDrawFrame()

    override fun onSurfaceCreated(gl: GL10?, config: EGLConfig?) {
        Log.i(TAG, "onSurfaceCreated called")

        try {
            // CRITICAL: Set AssetManager FIRST before any other native calls
            nativeSetAssetManager(context.assets)
            Log.i(TAG, "AssetManager set successfully")

            // Now initialize OpenGL
            nativeOnSurfaceCreated()
            Log.i(TAG, "Native surface created successfully")

            isInitialized = true
        } catch (e: Exception) {
            Log.e(TAG, "Error in onSurfaceCreated: ${e.message}")
            e.printStackTrace()
            isInitialized = false
        }
    }

    override fun onSurfaceChanged(gl: GL10?, width: Int, height: Int) {
        Log.i(TAG, "onSurfaceChanged: ${width}x${height}")

        if (!isInitialized) {
            Log.w(TAG, "Skipping onSurfaceChanged - not initialized")
            return
        }

        try {
            nativeOnSurfaceChanged(width, height)
        } catch (e: Exception) {
            Log.e(TAG, "Error in onSurfaceChanged: ${e.message}")
            e.printStackTrace()
        }
    }

    override fun onDrawFrame(gl: GL10?) {
        if (!isInitialized) {
            Log.w(TAG, "Skipping frame - not initialized")
            // Draw simple color to show something is happening
            gl?.glClearColor(1.0f, 0.0f, 0.0f, 1.0f) // Red = error state
            gl?.glClear(GL10.GL_COLOR_BUFFER_BIT)
            return
        }

        try {
            if (firstFrame) {
                Log.i(TAG, "Drawing first frame")
                firstFrame = false
            }

            nativeOnDrawFrame()

        } catch (e: Exception) {
            Log.e(TAG, "Error in onDrawFrame: ${e.message}")
            e.printStackTrace()
        }
    }
}