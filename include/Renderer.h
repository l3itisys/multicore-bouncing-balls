#ifndef BOUNCING_BALLS_RENDERER_H
#define BOUNCING_BALLS_RENDERER_H

#include "Types.h"
#include <vector>

namespace sim {

class Renderer {
public:
    Renderer(int width, int height);
    ~Renderer();

    bool initialize();
    void cleanup();
    void render(const std::vector<Ball>& balls);
    bool shouldClose() const { return glfwWindowShouldClose(window); }

    void resizeFramebuffer(int width, int height);
    GLFWwindow* getWindow() const { return window; }

private:
    void drawCircle(float centerX, float centerY, float radius, uint32_t color);
    void setupViewport(int width, int height);  // Added this declaration

    static void framebufferSizeCallback(GLFWwindow* window, int width, int height);
    static void errorCallback(int error, const char* description);

    GLFWwindow* window;
    int width;
    int height;

    static constexpr int CIRCLE_SEGMENTS = 32;
};

} // namespace sim

#endif // BOUNCING_BALLS_RENDERER_H
