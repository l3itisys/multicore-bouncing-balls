// Renderer.cpp
#include "Renderer.h"
#include "Simulation.h"
#include "Ball.h"
#include <iostream>
#include <cmath>

Renderer::Renderer(int windowWidth, int windowHeight)
    : window_(nullptr), windowWidth_(windowWidth), windowHeight_(windowHeight) {}

Renderer::~Renderer() {
    if (window_) {
        glfwDestroyWindow(window_);
        glfwTerminate();
    }
}

bool Renderer::initialize() {
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW\n";
        return false;
    }

    window_ = glfwCreateWindow(windowWidth_, windowHeight_, "Bouncing Balls Simulation", nullptr, nullptr);
    if (!window_) {
        std::cerr << "Failed to create GLFW window\n";
        glfwTerminate();
        return false;
    }

    glfwMakeContextCurrent(window_);

    GLenum err = glewInit();
    if (GLEW_OK != err) {
        std::cerr << "Failed to initialize GLEW: "
                  << glewGetErrorString(err) << "\n";
        return false;
    }

    glViewport(0, 0, windowWidth_, windowHeight_);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0, windowWidth_, screenHeight_, 0, -1, 1); // Origin at top-left corner
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    return true;
}

void Renderer::render(const Simulation& simulation) {
    glClear(GL_COLOR_BUFFER_BIT);

    const auto& balls = simulation.getBalls();
    for (const auto& ball : balls) {
        renderBall(*ball);
    }

    glfwSwapBuffers(window_);
    glfwPollEvents();
}

void Renderer::renderBall(const Ball& ball) {
    // Convert integer color to RGB components
    int color = ball.getColor();
    float r = ((color >> 16) & 0xFF) / 255.0f;
    float g = ((color >> 8) & 0xFF) / 255.0f;
    float b = (color & 0xFF) / 255.0f;

    glColor3f(r, g, b);

    float x = ball.getX();
    float y = ball.getY();
    float radius = ball.getRadius();

    const int segments = 20;
    glBegin(GL_TRIANGLE_FAN);
    glVertex2f(x, y);
    for (int n = 0; n <= segments; ++n) {
        float angle = n * 2.0f * M_PI / segments;
        float dx = radius * cosf(angle);
        float dy = radius * sinf(angle);
        glVertex2f(x + dx, y + dy);
    }
    glEnd();
}

bool Renderer::shouldClose() const {
    return glfwWindowShouldClose(window_);
}

