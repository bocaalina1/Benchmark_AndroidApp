package com.example.myapplication

import android.os.Bundle
import androidx.appcompat.app.AppCompatActivity
import com.example.myapplication.databinding.ActivityOpenglBinding // Import the new binding class

class OpenGLActivity : AppCompatActivity() {


    private lateinit var binding: ActivityOpenglBinding

    public override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)

        binding = ActivityOpenglBinding.inflate(layoutInflater)
        setContentView(binding.root)
    }

   override fun onResume() {
        super.onResume()
        binding.glSurface.onResume()
    }

    override fun onPause() {
        super.onPause()
        binding.glSurface.onPause()
    }
}
