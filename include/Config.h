#ifndef BOUNCING_BALLS_CONFIG_H
#define BOUNCING_BALLS_CONFIG_H

#include <CL/opencl.hpp>
#include <cstdint>

namespace sim {
namespace config {

// Display configuration
struct Display {
    static constexpr int DEFAULT_WIDTH = 1400;
    static constexpr int DEFAULT_HEIGHT = 900;
    static constexpr double TARGET_FPS = 60.0;
    static constexpr double FRAME_TIME = 1.0 / TARGET_FPS;
    static constexpr int MSAA_SAMPLES = 4;  // Added MSAA sample count
    static constexpr bool VSYNC_ENABLED = true;
};

// Physics simulation configuration
struct Physics {
    static constexpr float RATE = 240.0f;
    static constexpr float DT = 1.0f / RATE;
    static constexpr float GRAVITY = 9.81f;
    static constexpr float RESTITUTION = 0.8f;
};

// Ball configuration
struct Balls {
    static constexpr int MIN_COUNT = 3;
    static constexpr int MAX_COUNT = 200;
    static constexpr int DEFAULT_COUNT = 50;
    static constexpr float MIN_RADIUS = 15.0f;
    static constexpr float MAX_RADIUS = 25.0f;
    static constexpr float VELOCITY_RANGE = 100.0f;

    static constexpr uint32_t COLORS[] = {
        0xFF0000FF,  // Red (RGBA)
        0x00FF00FF,  // Green (RGBA)
        0x0000FFFF,  // Blue (RGBA)
        0xFF00FFFF,  // Magenta (RGBA)
        0x00FFFFFF,  // Cyan (RGBA)
        0xFFFF00FF   // Yellow (RGBA)
    };
    static constexpr size_t COLOR_COUNT = sizeof(COLORS) / sizeof(COLORS[0]);
};

// OpenCL configuration
struct OpenCL {
    static constexpr size_t WORKGROUP_SIZE = 256;
    static constexpr const char* KERNEL_FILENAME = "simulation.cl";
};

// Error messages
struct Error {
    static constexpr const char* GLFW_INIT_FAILED = "Failed to initialize GLFW";
    static constexpr const char* GLEW_INIT_FAILED = "Failed to initialize GLEW";
    static constexpr const char* WINDOW_CREATE_FAILED = "Failed to create GLFW window";
};

} // namespace config
} // namespace sim

#endif // BOUNCING_BALLS_CONFIG_H
