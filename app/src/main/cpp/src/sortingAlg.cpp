//
// Created by bocaa on 11/10/2025.
//
#include <jni.h>
#include <string>
#include <vector>
#include <chrono>
#include <sstream>
#include <random>
#include <algorithm>

using namespace std;

struct SortMetrics {
    long assigments = 0;
    long comparison = 0;
    long long duration_ms = 0;
};

void bubbleSort(vector<int>&array, SortMetrics &metrics)
{
    int n = array.size();
    bool swapped;
    for (int i=0; i<n-1; i++)
    {
        swapped = false;
        for (int j=0; j<n-i-1; j++)
        {
            metrics.comparison++;
            if (array[j] > array[j + 1]) {
                metrics.assigments += 3;
                swap(array[j], array[j + 1]);
                swapped = true;
            }
        }
        if(!swapped)
            break;
    }
}

void maxHeapify (vector<int>&array, int n, int i, SortMetrics &metrics)
{
    int largest = i;
    int left = 2*i+1;
    int right = 2*i+2;

    if(left < n)
    {
        metrics.comparison++;
        if(array[left] > array[largest])
            largest = left;

    }
    if(right <n)
    {
        metrics.comparison++;
        if(array[right] > array[largest])
            largest = right;

    }
    if(largest!=i)
    {
        metrics.assigments += 3;
        swap(array[i], array[largest]);
        maxHeapify(array, n, largest, metrics);

    }
}

void HeapSort(vector<int>&array, SortMetrics &metrics)
{
    int n = array.size();
    for(int i=n/2-1; i>=0; i--)
        maxHeapify(array, n, i, metrics);
    for(int i = n-1; i>0; i--)
    {
        metrics.assigments +=3;
        swap(array[0], array[i]);
        maxHeapify(array, i, 0, metrics);
    }
}

extern "C" JNIEXPORT jstring JNICALL
Java_com_example_myapplication_testCpuWithSorting_runAdvanceSort(JNIEnv *env, jobject, jint arraySize)
{
    vector<int> original_data(arraySize);
    mt19937 gen(12345);
    uniform_int_distribution<int> dis(1, 1000000);
    for(int i=0; i< arraySize; i++)
    {
        original_data[i] = dis(gen);
    }

    //bubleSort
    vector<int> data_buble = original_data;
    SortMetrics metrics_buble;

    auto start = chrono::high_resolution_clock::now();
    bubbleSort(data_buble, metrics_buble);
    auto end = chrono::high_resolution_clock::now();

    metrics_buble.duration_ms = chrono::duration_cast<chrono::milliseconds>(end - start).count();

    //heapSort
    vector<int> data_heap = original_data;
    SortMetrics metrics_heap;

    start = chrono::high_resolution_clock::now();
    HeapSort(data_heap, metrics_heap);
    end = chrono::high_resolution_clock::now();

    metrics_heap.duration_ms = chrono::duration_cast<chrono::milliseconds>(end - start).count();

    long bubble_ops = metrics_buble.assigments + metrics_buble.comparison;
    long heap_ops = metrics_heap.assigments + metrics_heap.comparison;

    std::stringstream result_ss;
    result_ss << metrics_buble.duration_ms << "," << bubble_ops << ";";
    result_ss << metrics_heap.duration_ms << "," << heap_ops;

    return env->NewStringUTF(result_ss.str().c_str());
}


