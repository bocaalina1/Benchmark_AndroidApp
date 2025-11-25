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
    private val testsPerSize = 7

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        binding = ActivityTestCpuBinding.inflate(layoutInflater)
        setContentView(binding.root)
        title = "CPU Benchmark"

        setUpChart(binding.chartBubbleTime, "Bubble Sort - Time","Time (ms)")
        setUpChart(binding.chartOperations, "Bubble Sort - Operations","Operations")
        setUpChart(binding.chartTime, "Heap Sort - Time","Time (ms)")
        setUpChart(binding.chartHeapOps, "Heap Sort - Operations","Operations")
        setUpChart(binding.chartBaseline, "Bubble Sort - Baseline","Time (ms)")

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
        binding.progressBar.visibility = View.VISIBLE
        binding.resultsTextview.text = "Running..."
        binding.startBenchmarkButton.isEnabled = false

        val bubbleResults = mutableListOf<AverageBenchmarkResult>()
        val heapResults = mutableListOf<AverageBenchmarkResult>()

        GlobalScope.launch(Dispatchers.Default) {
            var totalTests = 0
            var completedTests = 0

            totalTests = arraySize.size * testsPerSize

            for (size in arraySize) {
                val bubbleTestResults = mutableListOf<BenchmarkResult>()
                val heapTestResults = mutableListOf<BenchmarkResult>()

                for (test in 1..testsPerSize) {
                    completedTests++

                    withContext(Dispatchers.Main) {
                        binding.resultsTextview.text =
                            "Running benchmarks...\n\n" +
                                    "Progress: $completedTests/$totalTests\n" +
                                    "Array Size: $size elements\n" +
                                    "Test: $test/$testsPerSize"
                    }

                    val resultString = runAdvanceSort(size)
                    val (bubbleResult, heapResult) = parseResults(resultString, size)
                    bubbleTestResults.add(bubbleResult)
                    heapTestResults.add(heapResult)

                    delay(50)
                }

                // Calculate averages for this size
                val bubbleAvg = calculateAverage(bubbleTestResults)
                val heapAvg = calculateAverage(heapTestResults)

                bubbleResults.add(bubbleAvg)
                heapResults.add(heapAvg)
            }

            withContext(Dispatchers.Main) {
                displayResults(bubbleResults, heapResults)
                displayGraphs(bubbleResults, heapResults)
                binding.progressBar.visibility = View.GONE
                binding.startBenchmarkButton.isEnabled = true
            }
        }
    }

    private fun calculateAverage(results: List<BenchmarkResult>): AverageBenchmarkResult {
        val avgTime = results.map { it.timeMs }.average()
        val avgOps = results.map { it.operations }.average()
        val stdDevTime = calculateStdDev(results.map { it.timeMs.toDouble() })
        val stdDevOps = calculateStdDev(results.map { it.operations.toDouble() })

        return AverageBenchmarkResult(
            algorithm = results[0].algorithm,
            arraySize = results[0].arraySize,
            avgTimeMs = avgTime,
            avgOperations = avgOps,
            stdDevTime = stdDevTime,
            stdDevOps = stdDevOps,
            testsRun = results.size
        )
    }

    private fun calculateStdDev(values: List<Double>): Double {
        val mean = values.average()
        val variance = values.map { (it - mean) * (it - mean) }.average()
        return kotlin.math.sqrt(variance)
    }

    private fun parseResults(result: String, arraySize: Int): Pair<BenchmarkResult, BenchmarkResult> {
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

    private fun displayResults(bubbleResults: List<AverageBenchmarkResult>, heapResults: List<AverageBenchmarkResult>) {
        val sb = StringBuilder()
        sb.append("RESULTS (Avg of $testsPerSize tests)\n")
        sb.append("════════════════════════════════════\n\n")

        sb.append("BUBBLE SORT\n")
        sb.append("─────────────────────────────────\n")
        sb.append("Size    Time(ms)  Ops\n")
        for (result in bubbleResults) {
            sb.append(String.format("%-7d %-9.1f %.0f\n",
                result.arraySize,
                result.avgTimeMs,
                result.avgOperations
            ))
        }

        sb.append("\nHEAP SORT\n")
        sb.append("─────────────────────────────────\n")
        sb.append("Size    Time(ms)  Ops\n")
        for (result in heapResults) {
            sb.append(String.format("%-7d %-9.1f %.0f\n",
                result.arraySize,
                result.avgTimeMs,
                result.avgOperations
            ))
        }

        sb.append("\nCOMPARISON\n")
        sb.append("─────────────────────────────────\n")
        sb.append("Size    Bubble   Heap   Speedup\n")
        for (i in bubbleResults.indices) {
            val bubbleTime = bubbleResults[i].avgTimeMs
            val heapTime = heapResults[i].avgTimeMs
            val speedup = String.format("%.2fx", bubbleTime / heapTime)

            sb.append(String.format("%-7d %-8.1f %-6.1f %s\n",
                bubbleResults[i].arraySize,
                bubbleTime,
                heapTime,
                speedup
            ))
        }

        // Find best bubble sort size
        val bestBubbleResult = bubbleResults.minByOrNull { it.avgTimeMs }
        val bestBubbleTime = bestBubbleResult?.avgTimeMs ?: 0.0

        sb.append("\nBASELINE ANALYSIS\n")
        sb.append("─────────────────────────────────\n")
        sb.append("Best Bubble: ${bestBubbleResult?.arraySize}(${bestBubbleTime.toLong()}ms)\n\n")
        sb.append("Size   Variance\n")

        for (result in bubbleResults) {
            val variance = ((result.avgTimeMs - bestBubbleTime) / bestBubbleTime * 100)
            sb.append(String.format("%-6d %+6.1f%%\n",
                result.arraySize,
                variance
            ))
        }

        binding.resultsTextview.text = sb.toString()
    }

    private fun displayGraphs(bubbleResults: List<AverageBenchmarkResult>, heapResults: List<AverageBenchmarkResult>) {
        // Bubble Sort Time
        val bubbleTimeEntries = bubbleResults.mapIndexed { index, result ->
            Entry(index.toFloat(), result.avgTimeMs.toFloat())
        }

        // Heap Sort Time
        val heapTimeEntries = heapResults.mapIndexed { index, result ->
            Entry(index.toFloat(), result.avgTimeMs.toFloat())
        }

        // Bubble Sort Operations
        val bubbleOpsEntries = bubbleResults.mapIndexed { index, result ->
            Entry(index.toFloat(), result.avgOperations.toFloat())
        }

        // Heap Sort Operations
        val heapOpsEntries = heapResults.mapIndexed { index, result ->
            Entry(index.toFloat(), result.avgOperations.toFloat())
        }

        // Find best bubble sort time for EACH size (baseline for that size)
        val bestBubbleTimePerSize = bubbleResults.map { it.avgTimeMs }

        // Calculate variance from each size's baseline
        val varianceFromBaselineEntries = bubbleResults.mapIndexed { index, result ->
            val bestTime = bestBubbleTimePerSize.minOrNull() ?: 1.0
            val percentageDiff = ((result.avgTimeMs - bestTime) / bestTime * 100).toFloat()
            Entry(index.toFloat(), percentageDiff)
        }

        // Create individual charts
        createSingleChart(
            binding.chartBubbleTime,
            bubbleTimeEntries,
            "Bubble Sort - Time",
            Color.rgb(255, 102, 102)
        )

        createSingleChart(
            binding.chartOperations,
            bubbleOpsEntries,
            "Bubble Sort - Operations",
            Color.rgb(255, 102, 102)
        )

        createSingleChart(
            binding.chartTime,
            heapTimeEntries,
            "Heap Sort - Time",
            Color.rgb(102, 178, 255)
        )

        createSingleChart(
            binding.chartHeapOps,
            heapOpsEntries,
            "Heap Sort - Operations",
            Color.rgb(102, 178, 255)
        )

        // Create baseline chart showing variance from best bubble sort time
        createVarianceFromBaselineChart(
            binding.chartBaseline,
            bubbleTimeEntries,
            varianceFromBaselineEntries
        )

        binding.chartBubbleTime.visibility = View.VISIBLE
        binding.chartOperations.visibility = View.VISIBLE
        binding.chartTime.visibility = View.VISIBLE
        binding.chartHeapOps.visibility = View.VISIBLE
        binding.chartBaseline.visibility = View.VISIBLE
    }

    private fun createSingleChart(chart: LineChart, data: List<Entry>, label: String, color: Int) {
        val dataSet = LineDataSet(data, label).apply {
            this.color = color
            setCircleColor(color)
            lineWidth = 2.5f
            circleRadius = 5f
            setDrawCircleHole(false)
            valueTextSize = 9f
            setDrawValues(false)
            mode = LineDataSet.Mode.CUBIC_BEZIER
        }

        val lineData = LineData(dataSet)
        chart.data = lineData
        chart.invalidate()
    }

    private fun createVarianceFromBaselineChart(chart: LineChart, bubbleTimeData: List<Entry>, varianceData: List<Entry>) {
        val bubbleDataSet = LineDataSet(bubbleTimeData, "Bubble Time (ms)").apply {
            color = Color.rgb(255, 102, 102)
            setCircleColor(Color.rgb(255, 102, 102))
            lineWidth = 2.5f
            circleRadius = 5f
            setDrawCircleHole(false)
            valueTextSize = 9f
            setDrawValues(false)
            mode = LineDataSet.Mode.CUBIC_BEZIER
            axisDependency = com.github.mikephil.charting.components.YAxis.AxisDependency.LEFT
        }

        val varianceDataSet = LineDataSet(varianceData, "Variance from Best (%)").apply {
            color = Color.rgb(76, 175, 80)
            setCircleColor(Color.rgb(76, 175, 80))
            lineWidth = 2.5f
            circleRadius = 5f
            setDrawCircleHole(false)
            valueTextSize = 9f
            setDrawValues(false)
            mode = LineDataSet.Mode.CUBIC_BEZIER
            axisDependency = com.github.mikephil.charting.components.YAxis.AxisDependency.RIGHT
        }

        val lineData = LineData(bubbleDataSet, varianceDataSet)
        chart.data = lineData

        // Enable right axis for variance
        chart.axisRight.isEnabled = true
        chart.axisRight.setDrawGridLines(true)
        chart.axisRight.axisMinimum = 0f

        chart.invalidate()
    }

    data class BenchmarkResult(
        val algorithm: String,
        val arraySize: Int,
        val timeMs: Long,
        val operations: Long
    )

    data class AverageBenchmarkResult(
        val algorithm: String,
        val arraySize: Int,
        val avgTimeMs: Double,
        val avgOperations: Double,
        val stdDevTime: Double,
        val stdDevOps: Double,
        val testsRun: Int
    )

    private external fun runAdvanceSort(arraySize: Int): String

    companion object {
        init {
            System.loadLibrary("myapplication")
        }
    }
}