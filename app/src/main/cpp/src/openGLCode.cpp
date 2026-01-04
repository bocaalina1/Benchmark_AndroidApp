//
//  openGLCode.cpp
//
#include <jni.h>
#include <android/log.h>
#include <android/asset_manager.h>
#include <android/asset_manager_jni.h>
#include <GLES3/gl3.h>
#include <EGL/egl.h>
#include <string>
#include <memory>
#include <vector>
#include <fstream>
#include <chrono>

// GLM Includes
#include <../glm/glm.hpp>
#include <../glm/gtc/matrix_transform.hpp>
#include <../glm/gtc/matrix_inverse.hpp>
#include <../glm/gtc/type_ptr.hpp>

// Project Includes
#include "../includes/Shader.hpp"
#include "../includes/Model3D.hpp"
#include "../includes/Camera.hpp"
#include "../includes/Animation.h"
#include "../includes/Animator.h"
#include "../includes/model_animation.h"

// Logging Macros
#define LOG_TAG "NativeGL"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

// Error Checking Macro
#define CHECK_GL_ERROR(op) \
    for (GLint error = glGetError(); error; error = glGetError()) { \
        LOGE("after %s() glError (0x%x)", op, error); \
    }

// ----------------------------------------------------------------------------
// GLOBAL VARIABLES
// ----------------------------------------------------------------------------
static AAssetManager* g_assetManager = nullptr;

static int g_screenWidth = 800;
static int g_screenHeight = 600;
const unsigned int SHADOW_WIDTH = 2048;
const unsigned int SHADOW_HEIGHT = 2048;

// Matrices & Light
static glm::mat4 g_model;
static glm::mat4 g_view;
static glm::mat4 g_projection;
static glm::mat3 g_normalMatrix;
static glm::mat4 g_lightRotation;
static glm::vec3 g_lightDir = glm::vec3(0.0f, 1.0f, 1.0f);
static glm::vec3 g_lightColor = glm::vec3(1.0f, 1.0f, 1.0f);
static float g_lightAngle = 0.0f;
static float g_angleY = 0.0f;
static float g_fogD = 0.0f;

// Camera
static std::unique_ptr<gps::Camera> g_camera;
static float g_cameraSpeed = 0.1f;

// Static Models
static std::unique_ptr<gps::Model3D> g_ground;
static std::unique_ptr<gps::Model3D> g_lightCube;
static std::unique_ptr<gps::Model3D> g_screenQuad;

// Animated Models
static gps::Model* g_animatedModel = nullptr;
static gps::Animation* g_animation = nullptr;
static gps::Animator* g_animator = nullptr;

// Shaders
static std::unique_ptr<gps::Shader> g_myCustomShader;
static std::unique_ptr<gps::Shader> g_lightShader;
static std::unique_ptr<gps::Shader> g_depthMapShader;
static std::unique_ptr<gps::Shader> g_animationShader;
static std::unique_ptr<gps::Shader> g_animationDepthShader;

// Shadow Map
static GLuint g_shadowMapFBO = 0;
static GLuint g_depthMapTexture = 0;

// Timing
static float g_deltaTime = 0.0f;
static float g_lastFrame = 0.0f;
static auto g_startTime = std::chrono::high_resolution_clock::now();

// Initialization state
static bool g_objectsLoaded = false;
static bool g_shadersLoaded = false;
static bool g_firstFrame = true;

// ----------------------------------------------------------------------------
// HELPER FUNCTIONS (C++)
// ----------------------------------------------------------------------------

float getCurrentTime() {
    auto currentTime = std::chrono::high_resolution_clock::now();
    return std::chrono::duration<float, std::chrono::seconds::period>(
            currentTime - g_startTime).count();
}

glm::mat4 computeLightSpaceTrMatrix() {
    glm::mat4 lightView = glm::lookAt(
            glm::vec3(g_lightRotation * glm::vec4(g_lightDir, 1.0f)),
            glm::vec3(0.0f),
            glm::vec3(0.0f, 1.0f, 0.0f));
    glm::mat4 lightProjection = glm::ortho(-10.0f, 10.0f, -10.0f, 10.0f, 0.1f, 20.0f);
    return lightProjection * lightView;
}

void initObjects() {
    LOGI("Loading objects...");

    if (!g_assetManager) {
        LOGE("AssetManager is NULL! Cannot load objects.");
        return;
    }

    try {
        std::string modelPath = "tft7b_lulu (1).glb";

        LOGI("Creating Model...");
        g_animatedModel = new gps::Model(modelPath);

        LOGI("Creating Animation...");
        g_animation = new gps::Animation(modelPath, g_animatedModel, g_assetManager);

        LOGI("Creating Animator...");
        g_animator = new gps::Animator(g_animation);

        g_objectsLoaded = true;
        LOGI("Animated model loaded successfully");
    } catch (const std::exception& e) {
        LOGE("Failed to load animated model: %s", e.what());
        g_objectsLoaded = false;
    } catch (...) {
        LOGE("Unknown error loading animated model");
        g_objectsLoaded = false;
    }
}

void initShaders() {
    LOGI("Loading shaders...");

    if (!g_assetManager) {
        LOGE("AssetManager is NULL! Cannot load shaders.");
        return;
    }

    try {
        LOGI("Loading animation shader...");
        g_animationShader = std::make_unique<gps::Shader>();
        g_animationShader->loadShader("shaders/animation.vert", "shaders/shaderStart.frag", g_assetManager);
        CHECK_GL_ERROR("animation shader load");

        LOGI("Loading animation depth shader...");
        g_animationDepthShader = std::make_unique<gps::Shader>();
        g_animationDepthShader->loadShader("shaders/animationDepth.vert", "shaders/animationDepth.frag", g_assetManager);
        CHECK_GL_ERROR("animation depth shader load");

        g_shadersLoaded = true;
        LOGI("Shaders loaded successfully");
    } catch (const std::exception& e) {
        LOGE("Failed to load shaders: %s", e.what());
        g_shadersLoaded = false;
    } catch (...) {
        LOGE("Unknown error loading shaders");
        g_shadersLoaded = false;
    }
}

void initUniforms() {
    // Setup projection
    float aspect = (float)g_screenWidth / (float)g_screenHeight;
    g_projection = glm::perspective(glm::radians(45.0f), aspect, 0.1f, 1000.0f);

    // Setup View
    g_view = g_camera->getViewMatrix();
}

void initFBO() {
    LOGI("Initializing FBO...");

    glGenFramebuffers(1, &g_shadowMapFBO);
    CHECK_GL_ERROR("glGenFramebuffers");

    glGenTextures(1, &g_depthMapTexture);
    CHECK_GL_ERROR("glGenTextures");

    glBindTexture(GL_TEXTURE_2D, g_depthMapTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, SHADOW_WIDTH, SHADOW_HEIGHT, 0, GL_DEPTH_COMPONENT, GL_UNSIGNED_INT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    CHECK_GL_ERROR("glTexImage2D/glTexParameteri");

    glBindFramebuffer(GL_FRAMEBUFFER, g_shadowMapFBO);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, g_depthMapTexture, 0);

    GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if (status != GL_FRAMEBUFFER_COMPLETE) {
        LOGE("Framebuffer not complete! Status: 0x%x", status);
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    CHECK_GL_ERROR("FBO setup");

    LOGI("FBO initialized successfully");
}

void renderScene() {
    // SAFETY: Check if everything is loaded
    if (!g_objectsLoaded || !g_shadersLoaded) {
        // Draw a simple blue screen to show we're alive
        glClearColor(0.0f, 0.0f, 1.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        return;
    }

    // 1. Update Animation Timing
    float currentFrame = getCurrentTime();
    g_deltaTime = currentFrame - g_lastFrame;
    g_lastFrame = currentFrame;

    if (g_animator) {
        g_animator->UpdateAnimation(g_deltaTime);
    }

    // 2. Clear Screen
    glViewport(0, 0, g_screenWidth, g_screenHeight);
    glClearColor(0.5f, 0.5f, 0.5f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    CHECK_GL_ERROR("glClear");

    // 3. Render Animated Model
    if (g_animatedModel && g_animationShader && g_animationShader->shaderProgram) {
        g_animationShader->useShaderProgram();
        CHECK_GL_ERROR("useShaderProgram");

        // --- LIGHTING SETUP ---
        GLint lightDirLoc = glGetUniformLocation(g_animationShader->shaderProgram, "lightDir");
        GLint lightColorLoc = glGetUniformLocation(g_animationShader->shaderProgram, "lightColor");

        if (lightDirLoc >= 0) {
            glUniform3fv(lightDirLoc, 1, glm::value_ptr(g_lightDir));
        }
        if (lightColorLoc >= 0) {
            glUniform3fv(lightColorLoc, 1, glm::value_ptr(g_lightColor));
        }
        CHECK_GL_ERROR("light uniforms");

        // Setup View & Projection
        g_view = g_camera->getViewMatrix();
        GLint viewLoc = glGetUniformLocation(g_animationShader->shaderProgram, "view");
        GLint projLoc = glGetUniformLocation(g_animationShader->shaderProgram, "projection");

        if (viewLoc >= 0) {
            glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(g_view));
        }
        if (projLoc >= 0) {
            glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(g_projection));
        }
        CHECK_GL_ERROR("view/projection uniforms");

        // Setup Model Matrix
        g_model = glm::mat4(1.0f);
        g_model = glm::translate(g_model, glm::vec3(0.0f, -0.5f, 0.0f));
        g_model = glm::rotate(g_model, glm::radians(g_angleY), glm::vec3(0.0f, 1.0f, 0.0f));
        g_model = glm::scale(g_model, glm::vec3(0.01f));

        GLint modelLoc = glGetUniformLocation(g_animationShader->shaderProgram, "model");
        if (modelLoc >= 0) {
            glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(g_model));
        }
        CHECK_GL_ERROR("model uniform");

        // Calculate Normal Matrix
        g_normalMatrix = glm::mat3(glm::transpose(glm::inverse(g_view * g_model)));
        GLint normalMatrixLoc = glGetUniformLocation(g_animationShader->shaderProgram, "normalMatrix");
        if (normalMatrixLoc >= 0) {
            glUniformMatrix3fv(normalMatrixLoc, 1, GL_FALSE, glm::value_ptr(g_normalMatrix));
        }
        CHECK_GL_ERROR("normal matrix uniform");

        // Send Bone Transforms
        if (g_animator) {
            auto transforms = g_animator->GetFinalBoneMatrices();
            for (int i = 0; i < transforms.size() && i < 100; ++i) {
                std::string name = "finalBonesMatrices[" + std::to_string(i) + "]";
                GLint boneLoc = glGetUniformLocation(g_animationShader->shaderProgram, name.c_str());
                if (boneLoc >= 0) {
                    glUniformMatrix4fv(boneLoc, 1, GL_FALSE, glm::value_ptr(transforms[i]));
                }
            }
            CHECK_GL_ERROR("bone transforms");
        }

        // Draw the model
        g_animatedModel->Draw(*g_animationShader);
        CHECK_GL_ERROR("Draw");
    }
}

// ----------------------------------------------------------------------------
// JNI INTERFACE (Must be extern "C")
// ----------------------------------------------------------------------------
extern "C" {

JNIEXPORT void JNICALL
Java_com_example_myapplication_MyGLRenderer_nativeSetAssetManager(
        JNIEnv* env, jobject obj, jobject assetManager) {

    g_assetManager = AAssetManager_fromJava(env, assetManager);
    if (g_assetManager) {
        // IMPORTANT: Pass manager to Model class static helper
        gps::Model::SetAssetManager(g_assetManager);
        LOGI("AssetManager loaded successfully");
    } else {
        LOGE("Failed to load AssetManager");
    }
}

JNIEXPORT void JNICALL
Java_com_example_myapplication_MyGLRenderer_nativeOnSurfaceCreated(
        JNIEnv* env, jobject obj) {

    LOGI("nativeOnSurfaceCreated called");

    // Check AssetManager
    if (!g_assetManager) {
        LOGE("CRITICAL: AssetManager not set! Call nativeSetAssetManager first!");
        return;
    }

    // OpenGL Init
    glClearColor(0.5f, 0.5f, 0.5f, 1.0f);
    glEnable(GL_DEPTH_TEST);
    CHECK_GL_ERROR("Initial GL setup");

    // Init Camera
    g_camera = std::make_unique<gps::Camera>(
            glm::vec3(0.0f, 2.0f, 5.0f),
            glm::vec3(0.0f, 1.0f, 0.0f),
            glm::vec3(0.0f, 1.0f, 0.0f));
    LOGI("Camera initialized");

    // Initialize in order
    initObjects();
    initShaders();
    initUniforms();
    initFBO();

    LOGI("Surface creation complete. Objects loaded: %s, Shaders loaded: %s",
         g_objectsLoaded ? "YES" : "NO",
         g_shadersLoaded ? "YES" : "NO");
}

JNIEXPORT void JNICALL
Java_com_example_myapplication_MyGLRenderer_nativeOnSurfaceChanged(
        JNIEnv* env, jobject obj, jint width, jint height) {

    LOGI("nativeOnSurfaceChanged: %dx%d", width, height);

    g_screenWidth = width;
    g_screenHeight = height;
    glViewport(0, 0, width, height);
    CHECK_GL_ERROR("glViewport");

    float aspect = (float)width / (float)height;
    g_projection = glm::perspective(glm::radians(45.0f), aspect, 0.1f, 1000.0f);
}

JNIEXPORT void JNICALL
Java_com_example_myapplication_MyGLRenderer_nativeOnDrawFrame(
        JNIEnv* env, jobject obj) {

    // Simple first frame check
    if (g_firstFrame) {
        LOGI("Drawing first frame");
        g_firstFrame = false;
    }

    // Uncomment to rotate model:
    // g_angleY += 0.5f;

    renderScene();
}

}