#include "Renderer.h"
#include "Config.h"
#include <stdexcept>
#include <cmath>
#include <sstream>
#include <iomanip>
#include <iostream>
#include <algorithm>

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

    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebufferSizeCallback);

    // Initialize GLEW
    glewExperimental = GL_TRUE;
    if (glewInit() != GLEW_OK) {
        throw std::runtime_error("Failed to initialize GLEW");
    }

    setupOpenGL();

    // Enable VSync
    glfwSwapInterval(1);

    std::cout << "OpenGL Renderer initialized:\n"
              << "  Version: " << glGetString(GL_VERSION) << "\n"
              << "  Vendor: " << glGetString(GL_VENDOR) << "\n"
              << "  Renderer: " << glGetString(GL_RENDERER) << "\n\n";

    return true;
}

void Renderer::setupOpenGL() {
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
}

void Renderer::render(const std::vector<Ball>& balls, double fps) {
    // Clear the screen
    glClear(GL_COLOR_BUFFER_BIT);
    glLoadIdentity();

    // Draw balls
    drawBalls(balls);

    // Draw FPS counter
    renderFPS(fps);

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
    float r = ((color >> 24) & 0xFF) / 255.0f;
    float g = ((color >> 16) & 0xFF) / 255.0f;
    float b = ((color >> 8) & 0xFF) / 255.0f;

    glColor4f(r, g, b, alpha);
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
    std::stringstream ss;
    ss << "FPS: " << std::fixed << std::setprecision(1) << fps;

    glColor4f(1.0f, 1.0f, 1.0f, 1.0f); // White text
    drawText(ss.str(), 10.0f, 20.0f, TEXT_SCALE);
}

void Renderer::drawText(const std::string& text, float x, float y, float scale) {
    // Simple bitmap text rendering using immediate mode
    // For better text rendering, consider using a library like FreeType
    glPushMatrix();
    glTranslatef(x, y, 0.0f);
    glScalef(scale, scale, 1.0f);

    for (char c : text) {
        // Simple character rendering (placeholder)
        // Replace with actual text rendering logic as needed
        // Here, we'll just draw points as placeholders
        glBegin(GL_POINTS);
        glVertex2f(0, 0);
        glEnd();
        glTranslatef(10.0f, 0.0f, 0.0f); // Move to next character position
    }

    glPopMatrix();
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

