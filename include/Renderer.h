#ifndef RENDERER_H
#define RENDERER_H

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <vector>
#include <tuple>

class Renderer {
public:
    Renderer(int width, int height);
    ~Renderer();

    bool initialize();
    void render(const std::vector<std::tuple<float, float, float, int>>& renderingData);
    bool shouldClose();

    int getWidth() const;
    int getHeight() const;

private:
    GLFWwindow* window_;
    int width_;
    int height_;

    void drawBall(float x, float y, float radius, int color);
};

#endif // RENDERER_H

