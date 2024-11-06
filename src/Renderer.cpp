#include "Renderer.h"
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
    : width(width_), height(height_), window(nullptr) {
}

Renderer::~Renderer() {
    if (window) {
        glfwDestroyWindow(window);
    }
}

bool Renderer::initialize() {
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

    // Set background color (white)
    glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
}

void Renderer::render(const std::vector<Ball>& balls, double fps) {
    // Clear the screen
    glClear(GL_COLOR_BUFFER_BIT);
    glLoadIdentity();

    // Draw all balls with Z-ordering (larger balls first)
    std::vector<const Ball*> sortedBalls;
    sortedBalls.reserve(balls.size());
    for (const auto& ball : balls) {
        sortedBalls.push_back(&ball);
    }

    // Sort by radius (descending) for proper transparency
    std::sort(sortedBalls.begin(), sortedBalls.end(),
              [](const Ball* a, const Ball* b) { return a->radius > b->radius; });

    // Draw balls
    for (const Ball* ball : sortedBalls) {
        drawBall(*ball);
    }

    // Draw FPS counter
    renderFPS(fps);

    // Swap buffers and poll events
    glfwSwapBuffers(window);
    glfwPollEvents();
}

void Renderer::drawBall(const Ball& ball) {
    // Draw filled circle
    drawCircle(
        ball.position.x,
        ball.position.y,
        ball.radius,
        ball.color,
        0.7f  // Alpha for main ball
    );

    // Draw outline with slightly darker color
    glLineWidth(2.0f);
    glBegin(GL_LINE_LOOP);

    float r = ((ball.color >> 16) & 0xFF) / 255.0f * 0.8f;
    float g = ((ball.color >> 8) & 0xFF) / 255.0f * 0.8f;
    float b = (ball.color & 0xFF) / 255.0f * 0.8f;
    glColor4f(r, g, b, 0.7f);

    for (int i = 0; i < CIRCLE_SEGMENTS; ++i) {
        float angle = i * 2.0f * PI / CIRCLE_SEGMENTS;
        float x = ball.position.x + ball.radius * std::cos(angle);
        float y = ball.position.y + ball.radius * std::sin(angle);
        glVertex2f(x, y);
    }

    glEnd();
}

void Renderer::drawCircle(float x, float y, float radius, uint32_t color, float alpha) {
    float r = ((color >> 16) & 0xFF) / 255.0f;
    float g = ((color >> 8) & 0xFF) / 255.0f;
    float b = (color & 0xFF) / 255.0f;

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

    glColor4f(0.0f, 0.0f, 0.0f, 1.0f); // Black text
    drawText(ss.str(), 10.0f, 20.0f, TEXT_SCALE);
}

void Renderer::drawText(const std::string& text, float x, float y, float scale) {
    glPushMatrix();
    glTranslatef(x, y, 0.0f);
    glScalef(scale, scale, 1.0f);

    glLineWidth(2.0f);
    float charWidth = 8.0f;

    for (char c : text) {
        glBegin(GL_LINES);
        switch (c) {
            case 'F':
                glVertex2f(0, 0); glVertex2f(0, 10);
                glVertex2f(0, 0); glVertex2f(6, 0);
                glVertex2f(0, 5); glVertex2f(4, 5);
                break;
            case 'P':
                glVertex2f(0, 0); glVertex2f(0, 10);
                glVertex2f(0, 0); glVertex2f(6, 0);
                glVertex2f(6, 0); glVertex2f(6, 5);
                glVertex2f(0, 5); glVertex2f(6, 5);
                break;
            case 'S':
                glVertex2f(6, 0); glVertex2f(0, 0);
                glVertex2f(0, 0); glVertex2f(0, 5);
                glVertex2f(0, 5); glVertex2f(6, 5);
                glVertex2f(6, 5); glVertex2f(6, 10);
                glVertex2f(6, 10); glVertex2f(0, 10);
                break;
            case ':':
                glVertex2f(2, 3); glVertex2f(2, 4);
                glVertex2f(2, 7); glVertex2f(2, 8);
                break;
            case '.':
                glVertex2f(2, 0); glVertex2f(2, 1);
                break;
            case '0': case '1': case '2': case '3': case '4':
            case '5': case '6': case '7': case '8': case '9':
                glVertex2f(0, 0); glVertex2f(6, 0);
                glVertex2f(0, 0); glVertex2f(0, 10);
                glVertex2f(6, 0); glVertex2f(6, 10);
                glVertex2f(0, 10); glVertex2f(6, 10);
                break;
        }
        glEnd();

        glTranslatef(charWidth, 0.0f, 0.0f);
    }

    glPopMatrix();
}

void Renderer::framebufferSizeCallback(GLFWwindow* window, int width, int height) {
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
