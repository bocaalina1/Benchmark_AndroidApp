
cmake_minimum_required(VERSION 3.22.1)

# Declares the project name.
project("myapplication")

# It's good practice to explicitly set your C++ standard.
# Your code uses features from C++17.
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Creates your shared library and lists its source files.
add_library(myapplication SHARED
        myapplication.cpp
        sortingAlg.cpp
        memoryPerformance.cpp
        OpenGLCode.cpp
)

# MODERN PRACTICE: Apply include directories directly to the target.
# This is better than the old global `include_directories()` command.
target_include_directories(myapplication PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/glm
        ${CMAKE_CURRENT_SOURCE_DIR}/gltf  # Make sure this path is correct
)

# MODERN PRACTICE: Apply compile definitions directly to the target.
# This replaces the old `add_definitions()` commands.
#
# *** THE FIX IS HERE: TINYGLTF_NO_INCLUDE_JSON has been REMOVED ***
# to solve the 'nlohmann' compiler error.
target_compile_definitions(myapplication PRIVATE
        TINYGLTF_NOEXCEPTION
        TINYGLTF_NO_STB_IMAGE_WRITE
)

# Links your library against the necessary Android system libraries.
target_link_libraries(myapplication
        android
        log
        GLESv3
        EGL
)
