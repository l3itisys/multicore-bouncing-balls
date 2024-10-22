#include "Renderer.h"
#include <iostream>

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
        std::cerr << "Failed to initialize GLFW\n";
        return false;
    }

    window_ = glfwCreateWindow(width_, height_, "Bouncing Balls Simulation", nullptr, nullptr);
    if (!window_) {
        std::cerr << "Failed to create GLFW window\n";
        glfwTerminate();
        return false;
    }

    glfwMakeContextCurrent(window_);

    glewExperimental = GL_TRUE;
    GLenum err = glewInit();
    if (err != GLEW_OK) {
        std::cerr << "Failed to initialize GLEW: "
                  << glewGetErrorString(err) << "\n";
        return false;
    }

    glViewport(0, 0, width_, height_);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0, width_, height_, 0, -1, 1);
    glMatrixMode(GL_MODELVIEW);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    return true;
}

void Renderer::render(const std::vector<std::tuple<float, float, float, int>>& renderingData) {
    glClearColor(0, 0, 0, 1);
    glClear(GL_COLOR_BUFFER_BIT);

    for (const auto& data : renderingData) {
        float x, y, radius;
        int color;
        std::tie(x, y, radius, color) = data;
        drawBall(x, y, radius, color);
    }

    glfwSwapBuffers(window_);
    glfwPollEvents();
}

void Renderer::drawBall(float x, float y, float radius, int color) {
    glPushMatrix();
    glTranslatef(x, y, 0);

    // Convert color integer to RGBA
    float r = ((color >> 16) & 0xFF) / 255.0f;
    float g = ((color >> 8) & 0xFF) / 255.0f;
    float b = (color & 0xFF) / 255.0f;
    float a = 0.5f; // Transparency

    glColor4f(r, g, b, a);

    int numSegments = 36;
    glBegin(GL_TRIANGLE_FAN);
    glVertex2f(0, 0);
    for (int i = 0; i <= numSegments; ++i) {
        float angle = i * 2.0f * 3.1415926f / numSegments;
        float dx = cosf(angle) * radius;
        float dy = sinf(angle) * radius;
        glVertex2f(dx, dy);
    }
    glEnd();

    glPopMatrix();
}

bool Renderer::shouldClose() {
    return glfwWindowShouldClose(window_);
}

int Renderer::getWidth() const {
    return width_;
}

int Renderer::getHeight() const {
    return height_;
}

