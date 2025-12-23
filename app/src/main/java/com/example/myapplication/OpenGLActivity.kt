package com.example.myapplication

import android.os.Bundle
import android.util.Log
import androidx.appcompat.app.AppCompatActivity

class OpenGLActivity : AppCompatActivity() {

    private lateinit var glSurfaceView: MyGLSurfaceView

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)

        Log.i("OpenGLActivity", "Creating OpenGL Activity")

        try {
            // Initialize your custom GLSurfaceView
            glSurfaceView = MyGLSurfaceView(this)
            setContentView(glSurfaceView)

            Log.i("OpenGLActivity", "GLSurfaceView created and set")
        } catch (e: Exception) {
            Log.e("OpenGLActivity", "Failed to create GLSurfaceView: ${e.message}")
            e.printStackTrace()
            finish()
        }
    }

    override fun onResume() {
        super.onResume()
        if (::glSurfaceView.isInitialized) {
            glSurfaceView.onResume()
            Log.i("OpenGLActivity", "GLSurfaceView resumed")
        }
    }

    override fun onPause() {
        super.onPause()
        if (::glSurfaceView.isInitialized) {
            glSurfaceView.onPause()
            Log.i("OpenGLActivity", "GLSurfaceView paused")
        }
    }

    override fun onDestroy() {
        super.onDestroy()
        Log.i("OpenGLActivity", "Activity destroyed")
    }
}