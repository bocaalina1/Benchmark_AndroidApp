cmake_minimum_required(VERSION 3.22.1)

project("myapplication")

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Configure Assimp build options
set(BUILD_SHARED_LIBS ON CACHE BOOL "" FORCE)
set(ASSIMP_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(ASSIMP_BUILD_ASSIMP_TOOLS OFF CACHE BOOL "" FORCE)
set(ASSIMP_BUILD_SAMPLES OFF CACHE BOOL "" FORCE)
set(ASSIMP_BUILD_ZLIB ON CACHE BOOL "" FORCE)
set(ASSIMP_INSTALL OFF CACHE BOOL "" FORCE)

# Disable all importers by default
set(ASSIMP_BUILD_ALL_IMPORTERS_BY_DEFAULT OFF CACHE BOOL "" FORCE)
set(ASSIMP_BUILD_ALL_EXPORTERS_BY_DEFAULT OFF CACHE BOOL "" FORCE)

# Enable only the formats you need
set(ASSIMP_BUILD_GLTF_IMPORTER ON CACHE BOOL "" FORCE)
set(ASSIMP_BUILD_GLB_IMPORTER ON CACHE BOOL "" FORCE)
set(ASSIMP_BUILD_OBJ_IMPORTER ON CACHE BOOL "" FORCE)
set(ASSIMP_BUILD_FBX_IMPORTER ON CACHE BOOL "" FORCE)

# Android-specific settings for Assimp
set(ASSIMP_ANDROID_JNIIOSYSTEM ON CACHE BOOL "" FORCE)

# Add assimp subdirectory
add_subdirectory(assimp)

# Create your shared library
add_library(myapplication SHARED
        src/myapplication.cpp
        src/sortingAlg.cpp
        src/memoryPerformance.cpp
        src/Animation.cpp
        src/Animator.cpp
        src/Bone.cpp
        src/Camera.cpp
        src/Mesh.cpp
        src/model_animation.cpp
        src/Model3D.cpp
        src/Shader.cpp
        src/stb_image.cpp
        src/tiny_obj_loader.cpp
        src/openGLCode.cpp
)

# CRITICAL FIX: Prevent CMake from adding "d" suffix in debug mode
# This ensures output is always "libmyapplication.so"
set_target_properties(myapplication PROPERTIES DEBUG_POSTFIX "")

# Include directories
target_include_directories(myapplication PRIVATE
        ${CMAKE_SOURCE_DIR}
        ${CMAKE_SOURCE_DIR}/assimp/include
        ${CMAKE_BINARY_DIR}/assimp/include
        ${CMAKE_CURRENT_SOURCE_DIR}/glm
        ${CMAKE_CURRENT_SOURCE_DIR}/gltf
)

# Find required Android libraries
find_library(log-lib log)
find_library(android-lib android)
find_library(z-lib z)

# Compile definitions
target_compile_definitions(myapplication PRIVATE
        TINYGLTF_NOEXCEPTION
        TINYGLTF_NO_STB_IMAGE_WRITE
        GLM_ENABLE_EXPERIMENTAL # Fixes your GLM error globally
)

# Link all libraries
target_link_libraries(myapplication
        assimp
        ${android-lib}
        ${log-lib}
        ${z-lib}
        GLESv3
        EGL
)