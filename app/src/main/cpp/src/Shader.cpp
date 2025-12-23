//
//  Shader.cpp
//  Lab3
//
//  Target: Android (OpenGL ES 3.0)
//

#include "../includes/Shader.hpp"

// Log tag for Android Logcat
#define LOG_TAG "GPS_Shader"
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)

namespace gps {

    // Reads a file directly from the APK assets folder
    std::string Shader::readShaderFile(std::string fileName, AAssetManager* assetManager) {

        // Open the asset
        AAsset* asset = AAssetManager_open(assetManager, fileName.c_str(), AASSET_MODE_BUFFER);
        if (!asset) {
            LOGE("Error: Could not open shader file: %s", fileName.c_str());
            return "";
        }

        // Get file size
        off_t length = AAsset_getLength(asset);

        // Read data into a buffer
        std::vector<char> buffer(length + 1);
        int bytesRead = AAsset_read(asset, buffer.data(), length);

        AAsset_close(asset);

        if (bytesRead <= 0) {
            LOGE("Error: Empty or unreadable shader file: %s", fileName.c_str());
            return "";
        }

        // Null-terminate the string
        buffer[length] = '\0';

        return std::string(buffer.data());
    }

    void Shader::shaderCompileLog(GLuint shaderId) {
        GLint success;
        GLchar infoLog[512];

        // Check compilation status
        glGetShaderiv(shaderId, GL_COMPILE_STATUS, &success);
        if (!success) {
            glGetShaderInfoLog(shaderId, 512, NULL, infoLog);
            LOGE("Shader Compilation Error:\n%s", infoLog);
        }
    }

    void Shader::shaderLinkLog(GLuint shaderProgramId) {
        GLint success;
        GLchar infoLog[512];

        // Check link status
        glGetProgramiv(shaderProgramId, GL_LINK_STATUS, &success);
        if (!success) {
            glGetProgramInfoLog(shaderProgramId, 512, NULL, infoLog);
            LOGE("Shader Linking Error:\n%s", infoLog);
        }
    }

    void Shader::loadShader(std::string vertexShaderFileName, std::string fragmentShaderFileName, AAssetManager* assetManager) {

        // 1. Read shader source code from assets
        std::string v = readShaderFile(vertexShaderFileName, assetManager);
        std::string f = readShaderFile(fragmentShaderFileName, assetManager);

        // Safety check
        if (v.empty() || f.empty()) {
            LOGE("Failed to load shaders. Aborting.");
            return;
        }

        const GLchar* vertexShaderString = v.c_str();
        const GLchar* fragmentShaderString = f.c_str();

        // 2. Compile Vertex Shader
        GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
        glShaderSource(vertexShader, 1, &vertexShaderString, NULL);
        glCompileShader(vertexShader);
        shaderCompileLog(vertexShader);

        // 3. Compile Fragment Shader
        GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
        glShaderSource(fragmentShader, 1, &fragmentShaderString, NULL);
        glCompileShader(fragmentShader);
        shaderCompileLog(fragmentShader);

        // 4. Link Program
        this->shaderProgram = glCreateProgram();
        glAttachShader(this->shaderProgram, vertexShader);
        glAttachShader(this->shaderProgram, fragmentShader);
        glLinkProgram(this->shaderProgram);
        shaderLinkLog(this->shaderProgram);

        // 5. Cleanup individual shaders
        glDeleteShader(vertexShader);
        glDeleteShader(fragmentShader);

        LOGI("Shader loaded successfully: %s, %s", vertexShaderFileName.c_str(), fragmentShaderFileName.c_str());
    }

    void Shader::useShaderProgram() {
        if (this->shaderProgram != 0) {
            glUseProgram(this->shaderProgram);
        }
    }
}