package com.example.myapplication

import android.os.Bundle
import android.widget.TextView
import androidx.appcompat.app.AppCompatActivity
import java.io.File

class DeviceInfo : AppCompatActivity() {

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(R.layout.activity_device_info)
        title = "Informații Dispozitiv"
        val infoTextView = findViewById<TextView>(R.id.device_info_text)
        val deviceInfoString = getDeviceInfoFromJNI()
        val appChaceInfo = getAplicationCacheInfo()
        val fromApi = getHardwareAndBoardNames()

        val finalReport = "$deviceInfoString\n\n$appChaceInfo\n\n$fromApi\n\n"
        infoTextView.text = finalReport
    }
    private fun getAplicationCacheInfo(): String
    {
        val cacheSizeInBytes = calculatecacheSize(this.cacheDir)
        val cacheSizeInMB = cacheSizeInBytes / (1024 * 1024)
        return "APP CACHE INFO\n Size: $cacheSizeInMB MB"
    }
    private fun calculatecacheSize(directory: File): Long {
        return directory.walkBottomUp().fold(0L) { acc, file -> acc + file.length() }

    }
    private external fun getDeviceInfoFromJNI(): String

    private fun getHardwareAndBoardNames(): String {
        val hardware = android.os.Build.HARDWARE
        val board = android.os.Build.BOARD

        // Try to get the SoC model if we are on Android 12 (S) or newer
        var socModel = "N/A"
        if (android.os.Build.VERSION.SDK_INT >= android.os.Build.VERSION_CODES.S) {
            socModel = android.os.Build.SOC_MODEL
        }
        val cacheDatabaseResult = sentInfoToC(hardware, board)

        return """
        Build.HARDWARE: $hardware
        Build.BOARD: $board
        Build.SOC_MODEL: $socModel
        
        --- CACHE DATABASE RESULT ---
        $cacheDatabaseResult
    """.trimIndent()
    }
    private external fun sentInfoToC(hardware: String, board: String):String

    companion object {
        init {
            System.loadLibrary("myapplication")
        }
    }
}
