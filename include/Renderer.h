#ifndef RENDERER_H
#define RENDERER_H

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <tbb/concurrent_vector.h>
#include <vector>
#include "Ball.h"

class Renderer {
public:
    Renderer(int width, int height);
    ~Renderer();
    Renderer(const Renderer&) = delete;
    Renderer& operator=(const Renderer&) = delete;

    bool initialize();
    void render(const tbb::concurrent_vector<Ball*>& balls);
    bool shouldClose();
    int getWidth() const { return width_; }
    int getHeight() const { return height_; }

private:
    GLFWwindow* window_;
    int width_;
    int height_;
    void drawFilledCircle(float x, float y, float radius, const float* color);
};

#endif // RENDERER_H

