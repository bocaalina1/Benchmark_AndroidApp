#include <jni.h>
#include <android/log.h>
#include <GLES3/gl3.h>
#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/type_ptr.hpp"

#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, "OpenGLCode", __VA_ARGS__)

// --- Globals ---
GLuint program;
GLuint vao, vbo, ebo;
GLint mvpLocation;
int screenWidth = 1, screenHeight = 1;

// --- Shaders ---
const char* vertexShaderSrc =
        "#version 300 es\n"
        "layout(location = 0) in vec3 aPos;\n"
        "uniform mat4 uMVP;\n"
        "void main() {\n"
        "    gl_Position = uMVP * vec4(aPos, 1.0);\n"
        "}\n";

const char* fragmentShaderSrc =
        "#version 300 es\n"
        "precision mediump float;\n"
        "out vec4 FragColor;\n"
        "void main() {\n"
        "    FragColor = vec4(0.2, 0.7, 1.0, 1.0);\n"
        "}\n";

// --- Helpers ---
GLuint compileShader(GLenum type, const char* src) {
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &src, nullptr);
    glCompileShader(shader);

    GLint success;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        char log[512];
        glGetShaderInfoLog(shader, 512, nullptr, log);
        LOGI("Shader error: %s", log);
    }
    return shader;
}

GLuint buildProgram(const char* v, const char* f) {
    GLuint vs = compileShader(GL_VERTEX_SHADER, v);
    GLuint fs = compileShader(GL_FRAGMENT_SHADER, f);
    GLuint p = glCreateProgram();
    glAttachShader(p, vs);
    glAttachShader(p, fs);
    glLinkProgram(p);
    return p;
}

// --- JNI Entry Points ---
extern "C" {

JNIEXPORT void JNICALL
Java_com_example_myapplication_MyGLRenderer_nativeOnSurfaceCreated(
        JNIEnv*, jobject) {

    glEnable(GL_DEPTH_TEST);
    glClearColor(0.1f, 0.1f, 0.15f, 1.0f);

    program = buildProgram(vertexShaderSrc, fragmentShaderSrc);
    mvpLocation = glGetUniformLocation(program, "uMVP");

    // Cube vertices
    float vertices[] = {
            -1,-1,-1,   1,-1,-1,   1, 1,-1,   -1, 1,-1, // back
            -1,-1, 1,   1,-1, 1,   1, 1, 1,   -1, 1, 1  // front
    };

    // Cube indices
    unsigned int indices[] = {
            0,1,2, 2,3,0, // back
            4,5,6, 6,7,4, // front
            0,4,7, 7,3,0, // left
            1,5,6, 6,2,1, // right
            3,2,6, 6,7,3, // top
            0,1,5, 5,4,0  // bottom
    };

    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);

    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glGenBuffers(1, &ebo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glBindVertexArray(0);

    LOGI("Rotating cube initialized.");
}

JNIEXPORT void JNICALL
Java_com_example_myapplication_MyGLRenderer_nativeOnSurfaceChanged(
        JNIEnv*, jobject, jint width, jint height) {
    screenWidth = width;
    screenHeight = height;
    glViewport(0, 0, width, height);
}

JNIEXPORT void JNICALL
Java_com_example_myapplication_MyGLRenderer_nativeOnDrawFrame(
        JNIEnv*, jobject) {

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glUseProgram(program);

    // --- MVP ---
    static float angle = 0.f;
    angle += 0.01f;

    glm::mat4 model = glm::mat4(1.0f);

// rotation
    model = glm::rotate(model, angle, glm::vec3(0.5f, 1.0f, 0.0f));

// scaling (make cube smaller)
    model = glm::scale(model, glm::vec3(0.1f));   // <-- change this to your taste

    glm::mat4 view = glm::lookAt(
            glm::vec3(0, 0, 6),
            glm::vec3(0, 0, 0),
            glm::vec3(0, 1, 0));

    glm::mat4 proj = glm::perspective(glm::radians(45.0f),
                                      float(screenWidth) / float(screenHeight),
                                      0.1f,
                                      100.0f);

    glm::mat4 mvp = proj * view * model;

    glUniformMatrix4fv(mvpLocation, 1, GL_FALSE, glm::value_ptr(mvp));

    glBindVertexArray(vao);
    glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);
}

} // extern "C"
