package com.example.myapplication

import android.graphics.Color
import android.os.Build // Needed for BOARD and HARDWARE
import android.os.Bundle
import androidx.appcompat.app.AppCompatActivity
import com.example.myapplication.databinding.ActivityMemoryPerformanceBinding
import com.github.mikephil.charting.charts.LineChart
import com.github.mikephil.charting.components.XAxis
import com.github.mikephil.charting.data.Entry
import com.github.mikephil.charting.data.LineData
import com.github.mikephil.charting.data.LineDataSet
import kotlin.math.sqrt
import android.widget.TableRow
import android.widget.TextView
import android.view.Gravity
import android.view.View

data class BenchmarkResult(val n: Long, val timeBad: Double, val timeGood: Double)
class MemoryPerformanceActivity : AppCompatActivity() {

    private lateinit var binding: ActivityMemoryPerformanceBinding
    private var cacheSizes: LongArray? = null

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        binding = ActivityMemoryPerformanceBinding.inflate(layoutInflater)
        setContentView(binding.root)

        title = "Memory Benchmark"

        // 1. PASS HARDWARE INFO TO C++
        // Your C++ code requires (String hardware, String board)
        cacheSizes = getCacheSizeBytes(Build.HARDWARE, Build.BOARD)

        setupChart()

        binding.btnStart.setOnClickListener {
            // 2. DECIDE WHICH CACHE TO TARGET
            // Index 0 = L1, Index 1 = L2, Index 2 = L3
            // We try to grab L3. If it's 0 (doesn't exist), we fall back to L2.
            val l3Size = cacheSizes?.get(2) ?: 0L
            val l2Size = cacheSizes?.get(1) ?: 0L

            // Default to 2MB if everything fails so the app doesn't crash
            val targetCache = if (l3Size > 0) l3Size else if (l2Size > 0) l2Size else 2 * 1024 * 1024L

            runFullBenchmark(targetCache)
        }
    }

    private fun runFullBenchmark(detectedCacheSize: Long) {
        binding.progressBar.visibility = View.VISIBLE
        binding.btnStart.isEnabled = false
        binding.statusText.text = "Running Benchmark..."

        Thread {
            val entriesIJK = ArrayList<Entry>()
            val entriesIKJ = ArrayList<Entry>()
            val tableResults = ArrayList<BenchmarkResult>()

            // We test sizes relative to the detected cache (e.g., 0.5x the size, 2.0x the size)
            val sizeMultipliers = listOf(0.1, 0.25, 0.5, 0.75, 1.0, 1.25,1.5,1.75, 2.0, 4.0)

            for (multiplier in sizeMultipliers) {
                val targetBytes = (detectedCacheSize * multiplier).toLong()

                // Math: N = sqrt(Bytes / 24).
                // 24 comes from: 3 matrices * 8 bytes (sizeof long)
                val n = sqrt(targetBytes.toDouble() / 24.0).toLong()

                runOnUiThread {
                    binding.statusText.text =
                        "Testing Matrix: ${n}x${n} (${(targetBytes / 1024)} KB usage)..."
                }

                var totalTimeIJK = 0.0
                var totalTimeIKJ = 0.0

                val repeats = 5 // Reduced to 5 to make it faster for user
                for (k in 0 until repeats) {
                    val result = runMatrixBenchmark(n)
                    totalTimeIJK += result[0]
                    totalTimeIKJ += result[1]
                }

                val avgIJK = totalTimeIJK / repeats
                val avgIKJ = totalTimeIKJ / repeats

                // X-Axis = Multiplier (e.g. 1.0), Y-Axis = Time
                entriesIJK.add(Entry(multiplier.toFloat(), avgIJK.toFloat()))
                entriesIKJ.add(Entry(multiplier.toFloat(), avgIKJ.toFloat()))

                tableResults.add(
                    BenchmarkResult(
                        n,
                        avgIJK,
                        avgIKJ
                    ))

            }

            runOnUiThread {
                updateChartData(entriesIJK, entriesIKJ)
                binding.btnStart.isEnabled = true
                binding.statusText.text = "Done! Check the graph."
                binding.progressBar.visibility = View.GONE
                populateTable(tableResults)
            }

        }.start()
    }
    private fun populateTable(results: List<BenchmarkResult>) {
        for (res in results) {
            val row = TableRow(this)
            row.setPadding(0, 16, 0, 16) // Add vertical spacing

            // 1. Matrix Size Column
            val sizeText = TextView(this).apply {
                text = "${res.n} x ${res.n}"
                setTextColor(Color.BLACK)
            }

            // 2. Bad Order Time Column
            val badText = TextView(this).apply {
                text = String.format("%.4f s", res.timeBad)
                setTextColor(Color.RED)
                gravity = Gravity.END
            }

            // 3. Good Order Time Column
            val goodText = TextView(this).apply {
                text = String.format("%.4f s", res.timeGood)
                setTextColor(Color.parseColor("#388E3C")) // Dark Green
                gravity = Gravity.END
            }

            // Add views to row
            row.addView(sizeText)
            row.addView(badText)
            row.addView(goodText)

            // Add row to table
            binding.resultsTable.addView(row)
        }
    }
    private fun setupChart() {
        with(binding.lineChart) {
            description.isEnabled = false
            setTouchEnabled(true)
            isDragEnabled = true
            setScaleEnabled(true)
            setPinchZoom(true)

            // X-Axis formatting to make it look nice
            xAxis.position = XAxis.XAxisPosition.BOTTOM
            xAxis.granularity = 0.1f
            axisRight.isEnabled = false // Usually only need left axis
        }
    }

    private fun updateChartData(entriesIJK: ArrayList<Entry>, entriesIKJ: ArrayList<Entry>) {
        val dataSetIJK = LineDataSet(entriesIJK, "IJK (Bad Order)")
        dataSetIJK.color = Color.RED
        dataSetIJK.setCircleColor(Color.RED)
        dataSetIJK.lineWidth = 2f
        dataSetIJK.valueTextSize = 10f

        val dataIKJ = LineDataSet(entriesIKJ, "IKJ (Good Order)")
        dataIKJ.color = Color.BLUE
        dataIKJ.setCircleColor(Color.BLUE)
        dataIKJ.lineWidth = 2f
        dataIKJ.valueTextSize = 10f

        val data = LineData(dataSetIJK, dataIKJ)
        binding.lineChart.data = data
        binding.lineChart.invalidate() // Refresh
    }

    // 3. CORRECT JNI SIGNATURES
    // Matches your C++ code: getCacheSizeBytes(JNIEnv, obj, jstring, jstring)
    private external fun getCacheSizeBytes(hardware: String, board: String): LongArray?
    private external fun runMatrixBenchmark(matrixDimension: Long): DoubleArray

    companion object {
        init {
            System.loadLibrary("myapplication")
        }
    }
}