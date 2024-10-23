#include "Renderer.h"
#include <iostream>
#include <cmath>

Renderer::Renderer(int width, int height)
    : window_(nullptr), width_(width), height_(height) {}

Renderer::~Renderer() {
    if (window_) {
        glfwDestroyWindow(window_);
    }
    glfwTerminate();
}

bool Renderer::initialize() {
    if (!glfwInit()) {
        return false;
    }

    glfwWindowHint(GLFW_SAMPLES, 4); // Enable multisampling
    window_ = glfwCreateWindow(width_, height_, "Bouncing Balls Simulation", nullptr, nullptr);
    if (!window_) {
        glfwTerminate();
        return false;
    }

    glfwMakeContextCurrent(window_);

    glewExperimental = GL_TRUE;
    if (glewInit() != GLEW_OK) {
        std::cerr << "Failed to initialize GLEW\n";
        return false;
    }

    glEnable(GL_MULTISAMPLE);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glViewport(0, 0, width_, height_);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0, width_, height_, 0, -1, 1);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
    return true;
}

void Renderer::render(const tbb::concurrent_vector<Ball*>& balls) {
    glClear(GL_COLOR_BUFFER_BIT);
    glLoadIdentity();

    for (const Ball* ball : balls) {
        float x, y;
        ball->getPosition(x, y);

        int color = ball->getColor();
        float colorArray[4] = {
            ((color >> 16) & 0xFF) / 255.0f,
            ((color >> 8) & 0xFF) / 255.0f,
            (color & 0xFF) / 255.0f,
            0.5f // Set alpha to 0.5 for transparency
        };

        drawFilledCircle(x, y, ball->getRadius(), colorArray);
    }

    glfwSwapBuffers(window_);
    glfwPollEvents();
}

void Renderer::drawFilledCircle(float x, float y, float radius, const float* color) {
    const int numSegments = 32;
    const float angleStep = 2.0f * M_PI / numSegments;

    glColor4fv(color);
    glBegin(GL_TRIANGLE_FAN);
    glVertex2f(x, y);

    for (int i = 0; i <= numSegments; ++i) {
        float angle = i * angleStep;
        float px = x + radius * cosf(angle);
        float py = y + radius * sinf(angle);
        glVertex2f(px, py);
    }

    glEnd();
}

bool Renderer::shouldClose() {
    return glfwWindowShouldClose(window_);
}

