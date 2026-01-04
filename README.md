# Android NDK Sorting Benchmarker

A high-performance hybrid Android application that visualizes and benchmarks sorting algorithms. This project demonstrates the integration of **Kotlin** for UI and **C++ (NDK)** for raw computational logic, communicating via **JNI**.

![App Screenshot](path/to/screenshot.png) ## 🚀 Key Features

* **Hybrid Architecture:** UI layer built in Kotlin with computationally intensive sorting logic offloaded to C++ using the Android NDK.
* **Performance Benchmarking:** Compares execution speeds of native (C++) vs. managed (JVM) implementations.
* **Real-Time Visualization:** * **3D Animations:** Renders sorting steps using **OpenGL ES**.
    * **Live Metrics:** dynamic performance graphs powered by **MPAndroidChart**.
* **Memory Management:** Direct memory handling in C++ to minimize Garbage Collection (GC) overhead during heavy operations.

##  Tech Stack

* **Languages:** Kotlin, C++17
* **Build System:** CMake, Gradle (Groovy/KTS)
* **Graphics & UI:** OpenGL ES 3.0, MPAndroidChart, Android XML/Jetpack Compose
* **Core Concepts:** JNI (Java Native Interface), Multithreading, Cross-language compilation

##  Architecture

The app uses a bridge pattern to separate concerns:

1.  **Frontend (Kotlin):** Handles user input and draws the UI frame.
2.  **JNI Bridge:** Passes array pointers and sorting commands to the native layer.
3.  **Backend (C++):** Executes sorting algorithms (Quicksort, Mergesort, etc.) on raw memory addresses for maximum efficiency.

##  Setup & Installation

1.  **Prerequisites:** * Android Studio Ladybug (or newer)
    * NDK (Side-by-side) installed via SDK Manager
    * CMake 3.22.1+
2.  **Clone the repo:**
    ```bash
    git clone [https://github.com/yourusername/android-ndk-benchmarker.git](https://github.com/yourusername/android-ndk-benchmarker.git)
    ```
3.  **Build:**
    Open the project in Android Studio and sync Gradle. The `CMakeLists.txt` will automatically link the native C++ library.

