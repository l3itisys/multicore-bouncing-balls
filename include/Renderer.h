#ifndef BOUNCING_BALLS_RENDERER_H
#define BOUNCING_BALLS_RENDERER_H

#include "Types.h"
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <vector>
#include <memory>
#include <string>

namespace sim {

class Renderer {
public:
    Renderer(int width, int height);
    ~Renderer();

    Renderer(const Renderer&) = delete;
    Renderer& operator=(const Renderer&) = delete;

    bool initialize();
    void render(const std::vector<Ball>& balls, double fps);
    bool shouldClose() const;

    int getWidth() const { return width; }
    int getHeight() const { return height; }

private:
    class GLFWContext {
    public:
        GLFWContext();
        ~GLFWContext();
    };

    void setupOpenGL();
    void renderFPS(double fps);
    void drawBall(const Ball& ball);
    static void framebufferSizeCallback(GLFWwindow* window, int width, int height);

    void drawText(const std::string& text, float x, float y, float scale = 1.0f);
    void drawCircle(float x, float y, float radius, uint32_t color, float alpha);

    int width;
    int height;
    GLFWwindow* window;
    static GLFWContext glfw;  // Static GLFW initialization

    // Constants
    static constexpr int CIRCLE_SEGMENTS = 32;
    static constexpr float PI = 3.14159265358979323846f;
    static constexpr float TEXT_SCALE = 0.15f;
};

}

#endif // BOUNCING_BALLS_RENDERER_H
