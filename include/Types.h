#ifndef BOUNCING_BALLS_TYPES_H
#define BOUNCING_BALLS_TYPES_H

#include <array>
#include <cstdint>
#include <atomic>
#include <chrono>
#include <cmath>
#include <stdexcept>
#include <string>
#include <thread>

namespace cl {
    class Context;
    class CommandQueue;
    class Program;
    class Kernel;
    class Buffer;
}

namespace sim {

class GPUManager;
class Renderer;
class Simulation;

struct alignas(8) Vec2 {
    float x;
    float y;

    Vec2() : x(0.0f), y(0.0f) {}
    Vec2(float x_, float y_) : x(x_), y(y_) {}

    Vec2 operator+(const Vec2& other) const { return Vec2(x + other.x, y + other.y); }
    Vec2 operator-(const Vec2& other) const { return Vec2(x - other.x, y - other.y); }
    Vec2 operator*(float scalar) const { return Vec2(x * scalar, y * scalar); }
    Vec2& operator+=(const Vec2& other) { x += other.x; y += other.y; return *this; }
    Vec2& operator-=(const Vec2& other) { x -= other.x; y -= other.y; return *this; }
    Vec2& operator*=(float scalar) { x *= scalar; y *= scalar; return *this; }

    float dot(const Vec2& other) const { return x * other.x + y * other.y; }
    float lengthSquared() const { return dot(*this); }
    float length() const { return std::sqrt(lengthSquared()); }
    Vec2 normalized() const {
        float len = length();
        return len > 0 ? *this * (1.0f / len) : *this;
    }
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
    float dt;                 // Time step
    float gravity;            // Gravity constant
    float restitution;        // Collision restitution
    float padding;            // Alignment padding
    Vec2 screenDimensions;    // Screen dimensions
    Vec2 reserved;           // Reserved for future use
};

// Frame timing control
struct FrameTiming {
    static constexpr double TARGET_FPS = 30.0;
    static constexpr std::chrono::duration<double> FRAME_DURATION{1.0 / TARGET_FPS};

    std::chrono::high_resolution_clock::time_point lastFrameTime;
    std::atomic<double> currentFps{0.0};

    void updateFPS(std::chrono::high_resolution_clock::time_point now) {
        auto duration = now - lastFrameTime;
        currentFps.store(1.0 / std::chrono::duration<double>(duration).count(),
                        std::memory_order_relaxed);
        lastFrameTime = now;
    }

    double getFPS() const {
        return currentFps.load(std::memory_order_relaxed);
    }
};

// Thread synchronization helper
class ThreadSync {
public:
    void waitForComputation() {
        while (computationInProgress.load(std::memory_order_acquire)) {
            std::this_thread::yield();
        }
    }

    void startComputation() {
        computationInProgress.store(true, std::memory_order_release);
    }

    void endComputation() {
        computationInProgress.store(false, std::memory_order_release);
    }

private:
    std::atomic<bool> computationInProgress{false};
};

// Error handling class
class SimulationError : public std::runtime_error {
public:
    explicit SimulationError(const std::string& what_arg)
        : std::runtime_error(what_arg) {}
    explicit SimulationError(const char* what_arg)
        : std::runtime_error(what_arg) {}
};

}

#endif // BOUNCING_BALLS_TYPES_H
cmake_minimum_required(VERSION 3.10)
project(BouncingBalls)

add_subdirectory(multicore-bouncing-balls)
