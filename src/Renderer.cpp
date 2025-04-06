#include "Renderer.h"
#include "Config.h"
#include <stdexcept>
#include <cmath>
#include <sstream>
#include <iomanip>
#include <iostream>
#include <algorithm>

// Removed GLUT include
// #include <GL/glut.h>

namespace sim {

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

Renderer::Renderer(int width_, int height_)
    : width(width_), height(height_), window(nullptr), numBalls(0) {
}

Renderer::~Renderer() {
    if (window) {
        glfwDestroyWindow(window);
    }
}

bool Renderer::initialize(size_t numBalls_) {
    numBalls = numBalls_;

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
    glfwWindowHint(GLFW_SAMPLES, 4); // Enable antialiasing
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

    window = glfwCreateWindow(
        width, height,
        "Bouncing Balls Simulation (OpenCL)",
        nullptr, nullptr
    );

    if (!window) {
        throw std::runtime_error("Failed to create GLFW window");
    }

    // Do not make the context current here
    // Context will be made current in the render thread

    std::cout << "OpenGL Renderer initialized\n";

    return true;
}

void Renderer::setupOpenGL() {
    // Make sure the context is current
    if (!window || glfwGetCurrentContext() != window) {
        throw std::runtime_error("OpenGL context is not current in setupOpenGL");
    }

    // Initialize GLEW
    glewExperimental = GL_TRUE;
    GLenum err = glewInit();
    if (GLEW_OK != err) {
        throw std::runtime_error("Failed to initialize GLEW");
    }

    // Enable alpha blending
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // Enable antialiasing
    glEnable(GL_MULTISAMPLE);
    glEnable(GL_LINE_SMOOTH);
    glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);

    // Set up viewport and projection
    glViewport(0, 0, width, height);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0, width, height, 0, -1, 1);
    glMatrixMode(GL_MODELVIEW);

    // Set background color (dark gray for better visibility)
    glClearColor(0.2f, 0.2f, 0.2f, 1.0f);

    // Set swap interval (VSync)
    glfwSwapInterval(sim::config::Display::VSYNC_ENABLED ? 1 : 0);

    // Set framebuffer size callback
    glfwSetFramebufferSizeCallback(window, framebufferSizeCallback);

    std::cout << "OpenGL setup completed:\n"
              << "  Version: " << glGetString(GL_VERSION) << "\n"
              << "  Vendor: " << glGetString(GL_VENDOR) << "\n"
              << "  Renderer: " << glGetString(GL_RENDERER) << "\n\n";
}

void Renderer::render(const std::vector<Ball>& balls, double fps) {
    // Clear the screen
    glClear(GL_COLOR_BUFFER_BIT);
    glLoadIdentity();

    // Draw balls
    drawBalls(balls);

    // Optionally draw FPS counter
    // renderFPS(fps);

    // Swap buffers and poll events
    glfwSwapBuffers(window);
    glfwPollEvents();
}

void Renderer::drawBalls(const std::vector<Ball>& balls) {
    // Draw each ball
    for (const auto& ball : balls) {
        drawCircle(ball.position.x, ball.position.y, ball.radius, ball.color, 0.7f);
    }
}

void Renderer::drawCircle(float x, float y, float radius, uint32_t color, float alpha) {
    // Corrected color extraction (assuming ARGB format)
    float a = ((color >> 24) & 0xFF) / 255.0f * alpha;
    float r = ((color >> 16) & 0xFF) / 255.0f;
    float g = ((color >> 8) & 0xFF) / 255.0f;
    float b = (color & 0xFF) / 255.0f;

    glColor4f(r, g, b, a);
    glBegin(GL_TRIANGLE_FAN);

    // Center point
    glVertex2f(x, y);

    // Circle vertices
    for (int i = 0; i <= CIRCLE_SEGMENTS; ++i) {
        float angle = i * 2.0f * PI / CIRCLE_SEGMENTS;
        float px = x + radius * std::cos(angle);
        float py = y + radius * std::sin(angle);
        glVertex2f(px, py);
    }

    glEnd();
}

void Renderer::renderFPS(double fps) {
    // FPS rendering is disabled (no GLUT)
    // You can implement text rendering using another method if needed
}

void Renderer::drawText(const std::string& text, float x, float y, float scale) {
    // Text rendering is disabled (no GLUT)
    // You can implement text rendering using another method if needed
}

void Renderer::framebufferSizeCallback(GLFWwindow* window, int width, int height) {
    if (width == 0 || height == 0) return; // Avoid division by zero
    glViewport(0, 0, width, height);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0, width, height, 0, -1, 1);
    glMatrixMode(GL_MODELVIEW);
}

bool Renderer::shouldClose() const {
    return glfwWindowShouldClose(window);
}

} // namespace sim

