package com.example.myapplication

import android.os.Bundle
import androidx.appcompat.app.AppCompatActivity
import android.content.Intent
import com.example.myapplication.databinding.ActivityMainBinding



class MainActivity : AppCompatActivity() {
    private lateinit var binding: ActivityMainBinding
    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        binding = ActivityMainBinding.inflate(layoutInflater)
        setContentView(binding.root)

        binding.myButton.setOnClickListener {
            val intent = Intent(this, SecondActivity::class.java)
            startActivity(intent)
        }

        binding.deviceInfoButton.setOnClickListener {
            val intent = Intent(this, DeviceInfo::class.java)
            startActivity(intent)
        }
        binding.testCpuButton.setOnClickListener {
            val intent = Intent(this, testCpuWithSorting::class.java)
            startActivity(intent)
        }
        binding.memoryPerformanceButton.setOnClickListener {
            val intent = Intent(this, MemoryPerformanceActivity::class.java)
            startActivity(intent)
        }
        binding.openGLButton.setOnClickListener {
            val intent = Intent(this, OpenGLActivity::class.java)
            startActivity(intent)
        }
    }
}
