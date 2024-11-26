#ifndef BOUNCING_BALLS_TYPES_H
#define BOUNCING_BALLS_TYPES_H

// OpenCL includes
#define CL_HPP_ENABLE_EXCEPTIONS
#define CL_HPP_TARGET_OPENCL_VERSION 200
#define CL_HPP_MINIMUM_OPENCL_VERSION 200
#include <CL/opencl.hpp>

// OpenGL includes
#include <GL/glew.h>
#include <GLFW/glfw3.h>

// Standard includes
#include <array>
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cmath>
#include <cstdint>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

namespace sim {

// Forward declarations
class GPUManager;
class Simulation;

// Frame state enumeration
enum FrameState {
    FRAME_COMPUTING = 0,
    FRAME_READY = 1,
    FRAME_DISPLAYING = 2,
    FRAME_SWAPPING = 3
};

// 2D Vector structure matching OpenCL float2
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
    float dt;
    float gravity;
    float restitution;
    float padding;
    Vec2 screenDimensions;
    Vec2 reserved;
};

// Frame data structure for double buffering
struct Frame {
    std::vector<Ball> balls;
    std::chrono::high_resolution_clock::time_point timestamp;
};

// Frame timing control
struct FrameTiming {
    using clock = std::chrono::high_resolution_clock;
    using time_point = clock::time_point;
    using duration = std::chrono::duration<double>;

    static constexpr double TARGET_FPS = 30.0;
    static constexpr duration FRAME_DURATION{1.0 / TARGET_FPS};

    time_point lastFrameTime;
    std::atomic<double> currentFps{0.0};

    void updateFPS(time_point now) {
        auto duration = now - lastFrameTime;
        currentFps.store(1.0 / duration.count(), std::memory_order_relaxed);
        lastFrameTime = now;
    }

    double getFPS() const {
        return currentFps.load(std::memory_order_relaxed);
    }
};

// Performance monitoring
struct PerformanceMetrics {
    std::atomic<double> physicsTime{0.0};   // Physics computation time (ms)
    std::atomic<double> renderTime{0.0};    // Rendering time (ms)
    std::atomic<int> frameDrops{0};         // Number of dropped frames
    std::atomic<int> activeThreads{0};      // Number of active threads
    std::atomic<double> gpuUtilization{0.0};// GPU utilization percentage
    std::atomic<size_t> gpuMemoryUsage{0};  // GPU memory usage in bytes
    std::atomic<size_t> cpuMemoryUsage{0};  // CPU memory usage in bytes
};

// Thread synchronization helper
class ThreadSync {
public:
    void waitForComputation() {
        std::unique_lock<std::mutex> lock(mutex);
        computationComplete.wait(lock, [this] {
            return !computationInProgress.load(std::memory_order_acquire);
        });
    }

    void startComputation() {
        computationInProgress.store(true, std::memory_order_release);
    }

    void endComputation() {
        computationInProgress.store(false, std::memory_order_release);
        computationComplete.notify_one();
    }

    bool isComputing() const {
        return computationInProgress.load(std::memory_order_acquire);
    }

private:
    std::atomic<bool> computationInProgress{false};
    std::mutex mutex;
    std::condition_variable computationComplete;
};

// Error handling
class SimulationError : public std::runtime_error {
public:
    explicit SimulationError(const std::string& what_arg)
        : std::runtime_error(what_arg) {}
    explicit SimulationError(const char* what_arg)
        : std::runtime_error(what_arg) {}
};

// OpenGL interop structures
struct GLInteropObjects {
    GLuint vertexBuffer{0};     // OpenGL vertex buffer
    GLuint textureId{0};        // OpenGL texture
    cl::Buffer vertexBufferCL;  // OpenCL buffer for vertex data
    cl::Image2D textureCL;      // OpenCL image for texture
    bool initialized{false};     // Initialization flag
};

// Debug and profiling utilities (conditional compilation)
#ifdef _DEBUG
struct DebugStats {
    std::atomic<uint64_t> frameCount{0};
    std::atomic<uint64_t> updateCount{0};
    std::atomic<uint64_t> collisionCount{0};
    std::vector<double> frameTimes;
    std::mutex statsMutex;

    void addWarning(const std::string& warning);
    void clearWarnings();
    std::vector<std::string> getWarnings();

private:
    std::vector<std::string> warnings;
};
#endif

} // namespace sim

#endif // BOUNCING_BALLS_TYPES_H
