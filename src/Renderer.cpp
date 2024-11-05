#include "Renderer.h"
#include "GPUManager.h"
#include <stdexcept>
#include <cmath>
#include <sstream>
#include <iomanip>
#include <iostream>
#include <algorithm>

namespace sim {

// Initialize static GLFW context
Renderer::GLFWContext Renderer::glfw;

// GLFW Context management
Renderer::GLFWContext::GLFWContext() {
    if (!glfwInit()) {
        throw std::runtime_error("Failed to initialize GLFW");
    }
    std::cout << "GLFW initialized successfully\n";
}

Renderer::GLFWContext::~GLFWContext() {
    glfwTerminate();
}

Renderer::Renderer(int width_, int height_, GPUManager& gpuManager_)
    : width(width_), height(height_), window(nullptr), gpuManager(gpuManager_) {
}

Renderer::~Renderer() {
    if (window) {
        glfwDestroyWindow(window);
    }
    if (shaderProgram != 0) {
        glDeleteProgram(shaderProgram);
    }
    if (vao != 0) {
        glDeleteVertexArrays(1, &vao);
    }
    if (vbo != 0) {
        glDeleteBuffers(1, &vbo);
    }
}

bool Renderer::initialize(GLFWwindow* window_) {
    window = window_;
    // Configure GLFW window hints
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

    // Create window
    window = glfwCreateWindow(
        width, height,
        "Bouncing Balls Simulation (OpenCL)",
        nullptr, nullptr
    );

    if (!window) {
        throw std::runtime_error("Failed to create GLFW window");
    }

    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebufferSizeCallback);

    // Initialize GLEW
    glewExperimental = GL_TRUE;
    if (glewInit() != GLEW_OK) {
        throw std::runtime_error("Failed to initialize GLEW");
    }

    setupOpenGL();

    glViewport(0, 0, width, height);

    // Enable VSync
    glfwSwapInterval(1);

    std::cout << "OpenGL Renderer initialized:\n"
              << "  Version: " << glGetString(GL_VERSION) << "\n"
              << "  Vendor: " << glGetString(GL_VENDOR) << "\n"
              << "  Renderer: " << glGetString(GL_RENDERER) << "\n\n";

    return true;
}

void Renderer::cleanup() {
    if (shaderProgram != 0) {
        glDeleteProgram(shaderProgram);
        shaderProgram = 0;
    }
    if (vao != 0) {
        glDeleteVertexArrays(1, &vao);
        vao = 0;
    }
    if (vbo != 0) {
        glDeleteBuffers(1, &vbo);
        vbo = 0;
    }
}

void Renderer::setupOpenGL() {
    // Set up viewport and projection
    glViewport(0, 0, width, height);

    // Compile shaders
    const char* vertexShaderSource = R"(
        #version 330 core
        out vec2 TexCoords;
        void main() {
            const vec2 positions[4] = vec2[](
                vec2(-1.0, -1.0),
                vec2( 1.0, -1.0),
                vec2(-1.0,  1.0),
                vec2( 1.0,  1.0)
            );
            TexCoords = positions[gl_VertexID].xy * 0.5 + 0.5;
            gl_Position = vec4(positions[gl_VertexID], 0.0, 1.0);
        }
    )";

    const char* fragmentShaderSource = R"(
        #version 330 core
        in vec2 TexCoords;
        out vec4 FragColor;
        uniform sampler2D screenTexture;
        void main() {
            FragColor = texture(screenTexture, TexCoords);
        }
    )";

    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexShaderSource, nullptr);
    glCompileShader(vertexShader);

    // Check for compilation errors
    GLint success;
    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        throw std::runtime_error("Failed to compile vertex shader");
    }

    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentShaderSource, nullptr);
    glCompileShader(fragmentShader);

    // Check for compilation errors
    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        throw std::runtime_error("Failed to compile fragment shader");
    }

    // Link shaders into a program
    shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);

    // Check for linking errors
    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
    if (!success) {
        throw std::runtime_error("Failed to link shader program");
    }

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    // Create VAO and VBO
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);

    // No need to create VBO since we're using gl_VertexID

    glBindVertexArray(0);
}

void Renderer::render(double fps) {
    // Clear the screen
    glClear(GL_COLOR_BUFFER_BIT);

    // Render the texture
    renderTexture();

    // Draw FPS counter
    renderFPS(fps);

    // Swap buffers and poll events
    glfwSwapBuffers(window);
    glfwPollEvents();
}

void Renderer::renderTexture() {
    glUseProgram(shaderProgram);
    glBindVertexArray(vao);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, gpuManager.getTextureID());

    // Set the uniform
    glUniform1i(glGetUniformLocation(shaderProgram, "screenTexture"), 0);

    // Draw full-screen quad
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

    glBindVertexArray(0);
    glUseProgram(0);
}

void Renderer::renderFPS(double fps) {
    // Implement FPS rendering if needed
    // For simplicity, we'll omit text rendering in this example
}

void Renderer::framebufferSizeCallback(GLFWwindow* window, int width, int height) {
    glViewport(0, 0, width, height);
}

bool Renderer::shouldClose() const {
    return glfwWindowShouldClose(window);
}

} // namespace sim

