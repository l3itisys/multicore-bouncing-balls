#include "Renderer.h"
#include "Config.h"
#include <iostream>
#include <cmath>
#include <algorithm>

namespace sim {

Renderer::Renderer(int width_, int height_)
    : width(width_)
    , height(height_)
    , window(nullptr)
{
}

Renderer::~Renderer() {
    cleanup();
}

void Renderer::cleanup() {
    if (window) {
        glfwDestroyWindow(window);
        window = nullptr;
    }
    glfwTerminate();
}

bool Renderer::initialize() {
    // Initialize GLFW
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW" << std::endl;
        return false;
    }

    glfwSetErrorCallback(errorCallback);

    // Basic window hints
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
    glfwWindowHint(GLFW_SAMPLES, 4);

    // Create window
    window = glfwCreateWindow(width, height, "Bouncing Balls", nullptr, nullptr);
    if (!window) {
        std::cerr << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return false;
    }

    glfwMakeContextCurrent(window);

    // Initialize GLEW
    glewExperimental = GL_TRUE;
    if (glewInit() != GLEW_OK) {
        std::cerr << "Failed to initialize GLEW" << std::endl;
        return false;
    }

    // Clear any error flags
    while (glGetError() != GL_NO_ERROR);

    // Print OpenGL info
    std::cout << "\nOpenGL Context Info:" << std::endl;
    std::cout << "Version: " << glGetString(GL_VERSION) << std::endl;
    std::cout << "Vendor: " << glGetString(GL_VENDOR) << std::endl;
    std::cout << "Renderer: " << glGetString(GL_RENDERER) << std::endl;

    // Set up viewport and projection
    setupViewport(width, height);

    // Set callbacks
    glfwSetWindowUserPointer(window, this);
    glfwSetFramebufferSizeCallback(window, framebufferSizeCallback);

    // Configure OpenGL
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // Set background color
    glClearColor(0.2f, 0.2f, 0.2f, 1.0f);

    return true;
}

void Renderer::setupViewport(int width_, int height_) {
    // Update viewport size
    glViewport(0, 0, width_, height_);

    // Set up orthographic projection
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0.0, (double)width_, (double)height_, 0.0, -1.0, 1.0);

    // Reset modelview matrix
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    // Store dimensions
    width = width_;
    height = height_;

    // Verify viewport
    GLint viewport[4];
    glGetIntegerv(GL_VIEWPORT, viewport);
    std::cout << "Viewport set to: " << viewport[2] << "x" << viewport[3] << std::endl;
}

void Renderer::render(const std::vector<Ball>& balls) {
    // Clear the screen
    glClear(GL_COLOR_BUFFER_BIT);
    glLoadIdentity();

    // Draw solid color background
    glColor3f(0.2f, 0.2f, 0.2f);
    glBegin(GL_QUADS);
    glVertex2f(0.0f, 0.0f);
    glVertex2f((float)width, 0.0f);
    glVertex2f((float)width, (float)height);
    glVertex2f(0.0f, (float)height);
    glEnd();

    // Draw balls
    for (const auto& ball : balls) {
        drawCircle(ball.position.x, ball.position.y, ball.radius, ball.color);
    }

    // Swap buffers
    glfwSwapBuffers(window);
    glfwPollEvents();
}

void Renderer::drawCircle(float centerX, float centerY, float radius, uint32_t color) {
    // Extract color components
    float r = ((color >> 16) & 0xFF) / 255.0f;
    float g = ((color >> 8) & 0xFF) / 255.0f;
    float b = (color & 0xFF) / 255.0f;

    // Set color
    glColor3f(r, g, b);

    // Draw filled circle
    glBegin(GL_TRIANGLE_FAN);
    glVertex2f(centerX, centerY);  // Center point

    for (int i = 0; i <= CIRCLE_SEGMENTS; i++) {
        float angle = i * 2.0f * M_PI / CIRCLE_SEGMENTS;
        float x = centerX + radius * cosf(angle);
        float y = centerY + radius * sinf(angle);
        glVertex2f(x, y);
    }
    glEnd();

    // Draw outline
    glColor3f(r * 0.7f, g * 0.7f, b * 0.7f);
    glBegin(GL_LINE_LOOP);
    for (int i = 0; i < CIRCLE_SEGMENTS; i++) {
        float angle = i * 2.0f * M_PI / CIRCLE_SEGMENTS;
        float x = centerX + radius * cosf(angle);
        float y = centerY + radius * sinf(angle);
        glVertex2f(x, y);
    }
    glEnd();
}

void Renderer::resizeFramebuffer(int width_, int height_) {
    setupViewport(width_, height_);
}

void Renderer::framebufferSizeCallback(GLFWwindow* window, int width, int height) {
    auto* renderer = static_cast<Renderer*>(glfwGetWindowUserPointer(window));
    if (renderer) {
        renderer->resizeFramebuffer(width, height);
    }
}

void Renderer::errorCallback(int error, const char* description) {
    std::cerr << "GLFW Error (" << error << "): " << description << std::endl;
}

} // namespace sim
