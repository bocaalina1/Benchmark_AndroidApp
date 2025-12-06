package com.example.myapplication// In your MyGLRenderer.kt file
import android.content.Context
import android.content.res.AssetManager
import android.opengl.GLSurfaceView
import javax.microedition.khronos.egl.EGLConfig
import javax.microedition.khronos.opengles.GL10

class MyGLRenderer(private val context: Context) : GLSurfaceView.Renderer {

    // Declare the new native function to pass the AssetManager
   // private external fun nativeSetAssetManager(assetManager: AssetManager)

    private external fun nativeOnSurfaceCreated()
    private external fun nativeOnSurfaceChanged(width: Int, height: Int)
    private external fun nativeOnDrawFrame()

    override fun onSurfaceCreated(gl: GL10?, config: EGLConfig?) {
        // Pass the asset manager to the native code
        //nativeSetAssetManager(context.assets)
        nativeOnSurfaceCreated()
    }

    override fun onSurfaceChanged(gl: GL10?, width: Int, height: Int) {
        nativeOnSurfaceChanged(width, height)
    }

    override fun onDrawFrame(gl: GL10?) {
        nativeOnDrawFrame()
    }

    companion object {
        init {
            System.loadLibrary("myapplication")
        }
    }
}
