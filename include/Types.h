#ifndef BOUNCING_BALLS_TYPES_H
#define BOUNCING_BALLS_TYPES_H

#define CL_HPP_TARGET_OPENCL_VERSION 200
#define CL_HPP_MINIMUM_OPENCL_VERSION 200
#define CL_HPP_ENABLE_EXCEPTIONS

#include <CL/opencl.hpp>
#include <GL/glew.h>
#include <GLFW/glfw3.h>

namespace sim {

// 2D Vector structure matching OpenCL float2
struct alignas(8) Vec2 {
    float x;
    float y;
    Vec2() : x(0.0f), y(0.0f) {}
    Vec2(float x_, float y_) : x(x_), y(y_) {}
};

// Ball structure matching OpenCL kernel structure
struct alignas(32) Ball {
    Vec2 position;    // 8 bytes
    Vec2 velocity;    // 8 bytes
    float radius;     // 4 bytes
    float mass;       // 4 bytes
    uint32_t color;   // 4 bytes
    uint32_t padding; // 4 bytes for alignment
};

// Simulation constants matching OpenCL kernel structure
struct alignas(32) SimConstants {
    float dt;
    float gravity;
    float restitution;
    float padding;
    Vec2 screenDimensions;
    Vec2 reserved;
};

} // namespace sim

#endif // BOUNCING_BALLS_TYPES_H
