#ifndef BOUNCING_BALLS_RENDERER_H
#define BOUNCING_BALLS_RENDERER_H

#include "Types.h"
#include <vector>
#include <string>

namespace sim {

class Renderer {
public:
    Renderer(int width, int height);
    ~Renderer();

    bool initialize(size_t numBalls);
    void render(const std::vector<Ball>& balls, double fps = 0.0);
    bool shouldClose() const;
    GLFWwindow* getWindow() const { return window; }

    void setupOpenGL(); // Made public

private:
    // GLFW Context management
    class GLFWContext {
    public:
        GLFWContext();
        ~GLFWContext();
    private:
        GLFWContext(const GLFWContext&) = delete;
        GLFWContext& operator=(const GLFWContext&) = delete;
    };

    static GLFWContext glfw;

    void drawBalls(const std::vector<Ball>& balls);
    void drawCircle(float x, float y, float radius, uint32_t color, float alpha);
    void drawText(const std::string& text, float x, float y, float scale); // Added back
    void renderFPS(double fps); // Added back

    static void framebufferSizeCallback(GLFWwindow* window, int width, int height);

    GLFWwindow* window;
    int width;
    int height;

    // Data
    size_t numBalls;

    static constexpr int CIRCLE_SEGMENTS = 32;
    static constexpr float TEXT_SCALE = 0.15f;
    static constexpr float PI = 3.14159265358979323846f;
};

} // namespace sim

#endif // BOUNCING_BALLS_RENDERER_H

