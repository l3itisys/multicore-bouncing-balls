#ifndef BOUNCING_BALLS_RENDERER_H
#define BOUNCING_BALLS_RENDERER_H

#include "Types.h"
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <vector>
#include <memory>
#include <string>

namespace sim {

class GPUManager;

class Renderer {
public:
    Renderer(int width, int height, GPUManager& gpuManager);
    ~Renderer();

    // Prevent copying
    Renderer(const Renderer&) = delete;
    Renderer& operator=(const Renderer&) = delete;

    // Main interface
    bool initialize(GLFWwindow* window);
    void render(double fps);
    bool shouldClose() const;

    void cleanup();

    // Accessors
    int getWidth() const { return width; }
    int getHeight() const { return height; }

private:
    // GLFW context management
    class GLFWContext {
    public:
        GLFWContext();
        ~GLFWContext();
    };

    // Setup functions
    void setupOpenGL();
    void renderFPS(double fps);
    static void framebufferSizeCallback(GLFWwindow* window, int width, int height);

    // OpenGL rendering
    void renderTexture();

    // Member variables
    int width;
    int height;
    GLFWwindow* window;
    static GLFWContext glfw;  // Static GLFW initialization

    // Reference to GPUManager
    GPUManager& gpuManager;

    // Shader program
    GLuint shaderProgram = 0;
    GLuint vao = 0;
    GLuint vbo = 0;

    // Constants
    static constexpr float TEXT_SCALE = 0.15f;
};

} // namespace sim

#endif // BOUNCING_BALLS_RENDERER_H

