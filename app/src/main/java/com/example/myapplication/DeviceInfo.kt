
package com.example.myapplication

import android.graphics.Typeface
import android.os.Bundle
import android.view.View
import android.widget.LinearLayout
import android.widget.ProgressBar
import android.widget.TextView
import androidx.appcompat.app.AppCompatActivity
import com.example.myapplication.databinding.ActivityDeviceInfoBinding

class DeviceInfo : AppCompatActivity() {

    private lateinit var binding: ActivityDeviceInfoBinding

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        binding = ActivityDeviceInfoBinding.inflate(layoutInflater)
        setContentView(binding.root)
        title = "Informații Dispozitiv"


        val deviceInfoString = getDeviceInfoFromJNI()


        binding.loadingIndicator.visibility = View.GONE


        populateInfo(deviceInfoString)
    }

    private fun populateInfo(info: String) {

        val pairs = info.split(';').filter { it.isNotEmpty() }

        for (pair in pairs) {
            val parts = pair.split(':', limit = 2)
            if (parts.size == 2) {
                val key = parts[0]
                val value = parts[1]
                addRowToLayout(key, value)
            }
        }
    }

    private fun addRowToLayout(key: String, value: String) {
        val keyTextView = TextView(this).apply {
            text = key
            textSize = 16f
            typeface = Typeface.DEFAULT_BOLD // Text îngroșat
            layoutParams = LinearLayout.LayoutParams(
                LinearLayout.LayoutParams.WRAP_CONTENT,
                LinearLayout.LayoutParams.WRAP_CONTENT
            )
        }

        val valueTextView = TextView(this).apply {
            text = value
            textSize = 16f
            layoutParams = LinearLayout.LayoutParams(
                LinearLayout.LayoutParams.WRAP_CONTENT,
                LinearLayout.LayoutParams.WRAP_CONTENT
            ).apply {
                leftMargin = 16
            }
        }

        val rowLayout = LinearLayout(this).apply {
            orientation = LinearLayout.HORIZONTAL
            layoutParams = LinearLayout.LayoutParams(
                LinearLayout.LayoutParams.MATCH_PARENT,
                LinearLayout.LayoutParams.WRAP_CONTENT
            ).apply {
                topMargin = 12
            }
            addView(keyTextView)
            addView(valueTextView)
        }

        binding.infoContainer.addView(rowLayout)
    }

    private external fun getDeviceInfoFromJNI(): String

    companion object {
        init {
            System.loadLibrary("myapplication")
        }
    }
}
