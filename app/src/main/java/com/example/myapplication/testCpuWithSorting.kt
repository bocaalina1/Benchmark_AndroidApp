
package com.example.myapplication

import android.graphics.Color
import android.os.Bundle
import android.view.View
import androidx.appcompat.app.AppCompatActivity
import com.example.myapplication.databinding.ActivityTestCpuBinding
import com.github.mikephil.charting.charts.LineChart
import com.github.mikephil.charting.components.XAxis
import com.github.mikephil.charting.data.Entry
import com.github.mikephil.charting.data.LineData
import com.github.mikephil.charting.data.LineDataSet
import com.github.mikephil.charting.formatter.ValueFormatter
import kotlinx.coroutines.*

class testCpuWithSorting : AppCompatActivity() {

    private lateinit var binding: ActivityTestCpuBinding
    private val arraySize = listOf(1000, 2000, 3000, 4000, 5000, 6000, 7000, 8000, 9000, 10000, 15000, 20000)


    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        binding = ActivityTestCpuBinding.inflate(layoutInflater)
        setContentView(binding.root)
        title = "CPU Benchmark"

        setUpChart(binding.chartOperations, "Operations on Array Size","Operations")
        setUpChart(binding.chartTime, "Time to Sort Array Size","Time (ms)")

        binding.startBenchmarkButton.setOnClickListener {
            runBenchmark()
        }
    }
    private fun setUpChart(chart: LineChart, title: String, yAxis: String) {
    chart.apply{
        description.text = title
        description.textSize = 15f
        setTouchEnabled(true)
        isDragEnabled = true
        setScaleEnabled(true)
        setPinchZoom(true)

        xAxis.apply {
            position = XAxis.XAxisPosition.BOTTOM
            setDrawGridLines(true)
            granularity = 1f
            valueFormatter = object : ValueFormatter() {
                override fun getFormattedValue(value: Float): String {
                    val index = value.toInt()
                    return if (index >= 0 && index < arraySize.size) {
                        arraySize[index].toString()
                    } else {
                        ""
                    }
                }
            }
        }

        axisLeft.apply{
            setDrawGridLines(true)
            axisMinimum = 0f
        }
        axisRight.isEnabled = false
        legend.apply {
            isEnabled = true
            textSize = 13f
        }
    }
    }

    private fun runBenchmark() {
        // Show progress bar and disable button
        binding.progressBar.visibility = View.VISIBLE
        binding.resultsTextview.text = "Running..."
        binding.startBenchmarkButton.isEnabled = false

        val bubbleResults = mutableListOf<BenchmarkResult>()
        val heapResults = mutableListOf<BenchmarkResult>()

        // Run the C++ function on a background thread to avoid freezing the UI
        GlobalScope.launch(Dispatchers.Default) {
            // Call the native function with an array size of 10,000
            //val resultString = runAdvanceSort(10000)
            var currentProgress = 0
            for (size in arraySize) {
                currentProgress++
                // Switch back to the main thread to update the UI
                withContext(Dispatchers.Main) {
                    binding.resultsTextview.text =
                        "Running benchmarks...\n\n" +
                                "Progress: $currentProgress/${arraySize.size}\n" +
                                "Current size: $size elements"
                }
                val resultString = runAdvanceSort(size)
                val (bubbleResult, heapResult) = parseResults(resultString,size)
                bubbleResults.add(bubbleResult)
                heapResults.add(heapResult)
                delay(100)
            }
            // Switch back to main thread to update UI
            withContext(Dispatchers.Main) {
                displayResults(bubbleResults, heapResults)
                displayGraphs(bubbleResults, heapResults)
                binding.progressBar.visibility = View.GONE
                binding.startBenchmarkButton.isEnabled = true
            }

        }
    }
    private fun parseResults(result: String, arraySize: Int): Pair<BenchmarkResult, BenchmarkResult> {
        // Expected format: "bubble_ms,bubble_ops;heap_ms,heap_ops"
        val parts = result.split(';')

        val bubbleParts = parts[0].split(',')
        val heapParts = parts[1].split(',')

        val bubbleResult = BenchmarkResult(
            algorithm = "Bubble Sort",
            arraySize = arraySize,
            timeMs = bubbleParts[0].toLong(),
            operations = bubbleParts[1].toLong()
        )

        val heapResult = BenchmarkResult(
            algorithm = "Heap Sort",
            arraySize = arraySize,
            timeMs = heapParts[0].toLong(),
            operations = heapParts[1].toLong()
        )

        return Pair(bubbleResult, heapResult)
    }
    private fun displayResults(bubbleResults: List<BenchmarkResult>, heapResults: List<BenchmarkResult>) {
        val sb = StringBuilder()
        sb.append("=== BENCHMARK RESULTS ===\n\n")

        sb.append("BUBBLE SORT:\n")
        sb.append("-".repeat(50)).append("\n")
        for (result in bubbleResults) {
            sb.append("Size: ${result.arraySize} | ")
            sb.append("Time: ${result.timeMs}ms | ")
            sb.append("Ops: ${result.operations}\n")
        }

        sb.append("\n")
        sb.append("HEAP SORT:\n")
        sb.append("-".repeat(50)).append("\n")
        for (result in heapResults) {
            sb.append("Size: ${result.arraySize} | ")
            sb.append("Time: ${result.timeMs}ms | ")
            sb.append("Ops: ${result.operations}\n")
        }

        sb.append("\n")
        sb.append("PERFORMANCE COMPARISON:\n")
        sb.append("-".repeat(50)).append("\n")

        // Calculate averages
        val avgBubbleTime = bubbleResults.map { it.timeMs }.average()
        val avgHeapTime = heapResults.map { it.timeMs }.average()
        val avgBubbleOps = bubbleResults.map { it.operations }.average()
        val avgHeapOps = heapResults.map { it.operations }.average()

        sb.append("Avg Time - Bubble: ${avgBubbleTime.toLong()}ms, Heap: ${avgHeapTime.toLong()}ms\n")
        sb.append("Avg Ops - Bubble: ${avgBubbleOps.toLong()}, Heap: ${avgHeapOps.toLong()}\n")

        val timeDiff = ((avgBubbleTime / avgHeapTime - 1) * 100).toInt()
        val opsDiff = ((avgBubbleOps / avgHeapOps - 1) * 100).toInt()

        sb.append("\nHeap Sort is ${timeDiff} times faster\n")
        sb.append("Heap Sort uses ${opsDiff} times fewer operations\n")

        binding.resultsTextview.text = sb.toString()
    }

    private fun displayGraphs(bubbleResults: List<BenchmarkResult>, heapResults: List<BenchmarkResult>) {
        val bubbleOpsEntries = bubbleResults.mapIndexed { index, result ->
            Entry(index.toFloat(), result.operations.toFloat())
        }
        val heapOpsEntries = heapResults.mapIndexed { index, result ->
            Entry(index.toFloat(), result.operations.toFloat())
        }

        val bubbleTimeEntries = bubbleResults.mapIndexed { index, result ->
            Entry(index.toFloat(), result.timeMs.toFloat())
        }
        val heapTimeEntries = heapResults.mapIndexed { index, result ->
            Entry(index.toFloat(), result.timeMs.toFloat())
        }

        // Create Operations chart
        createChart(
            binding.chartOperations,
            bubbleOpsEntries,
            heapOpsEntries,
            "Bubble Sort",
            "Heap Sort",
            Color.rgb(255, 102, 102),  // Red
            Color.rgb(102, 178, 255)   // Blue
        )

        // Create Time chart
        createChart(
            binding.chartTime,
            bubbleTimeEntries,
            heapTimeEntries,
            "Bubble Sort",
            "Heap Sort",
            Color.rgb(255, 102, 102),  // Red
            Color.rgb(102, 178, 255)   // Blue
        )

        // Show charts
        binding.chartOperations.visibility = View.VISIBLE
        binding.chartTime.visibility = View.VISIBLE
    }

    private fun createChart(
        chart: LineChart,
        data1: List<Entry>,
        data2: List<Entry>,
        label1: String,
        label2: String,
        color1: Int,
        color2: Int
    ) {
        // Create datasets
        val dataSet1 = LineDataSet(data1, label1).apply {
            color = color1
            setCircleColor(color1)
            lineWidth = 2.5f
            circleRadius = 4f
            setDrawCircleHole(false)
            valueTextSize = 9f
            setDrawValues(false)
            mode = LineDataSet.Mode.CUBIC_BEZIER
        }

        val dataSet2 = LineDataSet(data2, label2).apply {
            color = color2
            setCircleColor(color2)
            lineWidth = 2.5f
            circleRadius = 4f
            setDrawCircleHole(false)
            valueTextSize = 9f
            setDrawValues(false)
            mode = LineDataSet.Mode.CUBIC_BEZIER
        }

        // Set data to chart
        val lineData = LineData(dataSet1, dataSet2)
        chart.data = lineData
        chart.invalidate()  // Refresh chart
    }

    /**
     * Data class to hold benchmark results
     */
    data class BenchmarkResult(
        val algorithm: String,
        val arraySize: Int,
        val timeMs: Long,
        val operations: Long
    )

    /**
     * Native function declaration
     */
    private external fun runAdvanceSort(arraySize: Int): String

    companion object {
        init {
            System.loadLibrary("myapplication")
        }
    }
}