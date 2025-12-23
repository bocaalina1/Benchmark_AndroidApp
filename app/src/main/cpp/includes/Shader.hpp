
#ifndef Shader_hpp
#define Shader_hpp

#include <string>
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>

#include <android/log.h>
#include <android/asset_manager.h>
#include <GLES3/gl3.h>

namespace gps {

    class Shader {

    public:
        GLuint shaderProgram;

        // Constructor/Destructor (Optional but recommended for cleanup)
        Shader() : shaderProgram(0) {}

        // Loads vertex and fragment shaders from the APK assets
        void loadShader(std::string vertexShaderFileName, std::string fragmentShaderFileName, AAssetManager* assetManager);

        void useShaderProgram();

    private:
        // Helper to read file content from assets
        std::string readShaderFile(std::string fileName, AAssetManager* assetManager);

        void shaderCompileLog(GLuint shaderId);
        void shaderLinkLog(GLuint shaderProgramId);
    };
}

#endif /* Shader_hpp */