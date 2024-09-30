#include "Renderer.h"
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
        std::cerr << "Failed to initialize GLFW" << std::endl;
        return false;
    }

    window_ = glfwCreateWindow(windowWidth_, windowHeight_, "Bouncing Balls Simulation", nullptr, nullptr);
    if (!window_) {
        std::cerr << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return false;
    }

    glfwMakeContextCurrent(window_);

    glewExperimental = GL_TRUE; // Needed for core profile
    if (glewInit() != GLEW_OK) {
        std::cerr << "Failed to initialize GLEW" << std::endl;
        return false;
    }

    glViewport(0, 0, windowWidth_, windowHeight_);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0, windowWidth_, windowHeight_, 0, -1, 1);
    glMatrixMode(GL_MODELVIEW);

    return true;
}

void Renderer::render(const Simulation& simulation) {
    glClear(GL_COLOR_BUFFER_BIT);
    glLoadIdentity();

    const auto& balls = simulation.getBalls();

    for (const auto& ball : balls) {
        renderBall(*ball);
    }

    glfwSwapBuffers(window_);
    glfwPollEvents();
}

void Renderer::renderBall(const Ball& ball) {
    float x = ball.getX();
    float y = ball.getY();
    float radius = ball.getRadius();
    int color = ball.getColor();

    float r = ((color >> 16) & 0xFF) / 255.0f;
    float g = ((color >> 8) & 0xFF) / 255.0f;
    float b = (color & 0xFF) / 255.0f;

    glColor3f(r, g, b);
    glBegin(GL_TRIANGLE_FAN);
    glVertex2f(x, y);
    for (int i = 0; i <= 360; i += 10) {
        float radian = i * 3.14159f / 180.0f;
        glVertex2f(x + cos(radian) * radius, y + sin(radian) * radius);
    }
    glEnd();

    // Optional: Draw velocity vector
    glColor3f(1.0f, 1.0f, 1.0f); // White color for velocity vector
    glBegin(GL_LINES);
    glVertex2f(x, y);
    glVertex2f(x + ball.getVx(), y + ball.getVy());
    glEnd();
}

bool Renderer::shouldClose() const {
    return glfwWindowShouldClose(window_);
}

