#ifndef RENDERER_H
#define RENDERER_H

#include "Simulation.h"
#include <GL/glew.h>
#include <GLFW/glfw3.h>

class Renderer {
public:
    Renderer(int windowWidth, int windowHeight);
    ~Renderer();

    bool initialize();
    void render(const Simulation& simulation);
    bool shouldClose() const;

private:
    void renderBall(const Ball& ball);

    GLFWwindow* window_;
    int windowWidth_;
    int windowHeight_;
};

#endif // RENDERER_H

