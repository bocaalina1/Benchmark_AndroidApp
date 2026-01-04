//
// Created by bocaa on 11/24/2025.
//

#include <jni.h>
#include <string>
#include <vector>
#include <chrono>
#include <sstream>
#include <android/log.h>
#include <random>

#define LOG_TAG "MatrixBenchmark"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)

using namespace std;
using namespace std::chrono;

void multiplyMatrices_IJK( long **A, long **B, long** C, int size)
{
    for (int i=0; i<size; i++) {
        for (int j = 0; j < size; j++) {
            C[i][j] = 0;
            for (int k = 0; k < size; k++) {
                C[i][j] += A[i][k] * B[k][j];
            }
        }
    }
}

void multiplyMatrices_IKJ( long **A, long **B, long** C, int size){
    for (int i = 0; i < size; i++) {
        for (int k = 0; k < size; k++) {
            for (int j = 0; j < size; j++)
                C[i][j] += A[i][j] * B[j][k];
        }
    }
}
extern "C" JNIEXPORT jdoubleArray JNICALL
Java_com_example_myapplication_MemoryPerformanceActivity_runMatrixBenchmark(JNIEnv *env, jobject, jlong cacheSize) {
    long **A = (long **) calloc(cacheSize, sizeof(long *));
    for (int i = 0; i < cacheSize; i++)
        A[i] = (long *) calloc(cacheSize, sizeof(long));

    long **B = (long **) calloc(cacheSize, sizeof(long *));
    for (int i = 0; i < cacheSize; i++)
        B[i] = (long *) calloc(cacheSize, sizeof(long));

    long **C = (long **) calloc(cacheSize, sizeof(long *));
    for (int i = 0; i < cacheSize; i++)
        C[i] = (long *) calloc(cacheSize, sizeof(long));

    mt19937 gen(12345);
    uniform_int_distribution<int> dis(1, 100);
    for(int i=0; i< cacheSize; i++)
    {
        for(int j=0; j<cacheSize; j++)
        {
            A[i][j] = dis(gen);
            B[i][j] = dis(gen);
        }
    }

    auto start1 = high_resolution_clock::now();
    multiplyMatrices_IJK(A, B, C, cacheSize);
    auto end1 = high_resolution_clock::now();

    for(int i = 0; i<cacheSize;i++)
    {
        for(int j=0; j< cacheSize; j++)
        {
            C[i][j] = 0;
        }
    }

    auto start2 = high_resolution_clock::now();
    multiplyMatrices_IKJ(A, B, C, cacheSize);
    auto end2 = high_resolution_clock::now();

    duration<double> time1 = end1 - start1;
    duration<double> time2 = end2 - start2;

    for (int i = 0; i < cacheSize; i++) {
        free(A[i]);
        free(B[i]);
        free(C[i]);
    }
    free(A);
    free(B);
    free(C);

    jdoubleArray  result = env->NewDoubleArray(2);
    jdouble times[2] = {time1.count(), time2.count()};
    env->SetDoubleArrayRegion(result, 0, 2, times);
    return result;
}


