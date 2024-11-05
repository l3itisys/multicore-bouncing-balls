#ifndef BOUNCING_BALLS_SIMULATION_H
#define BOUNCING_BALLS_SIMULATION_H

#include "Types.h"
#include "GPUManager.h"
#include "Timing.h"
#include <vector>
#include <thread>
#include <memory>
#include <mutex>

namespace sim {

class Simulation {
public:
    // Constructor and destructor
    Simulation(int numBalls, float screenWidth, float screenHeight);
    ~Simulation();

    // Delete copy operations
    Simulation(const Simulation&) = delete;
    Simulation& operator=(const Simulation&) = delete;

    // Control interface
    void start();
    void stop();
    std::vector<Ball> getBalls() const;
    double getCurrentFPS() const { return timing.getFPS(); }

private:
    // Thread functions
    void controlThreadFunc();      // Manages display updates at 30 FPS
    void computationThreadFunc();  // Manages GPU computations
    void initializeBalls(int numBalls);

    // Thread management functions
    void updateDisplay();      // Updates display at 30 FPS
    void updatePhysics();      // Updates physics on GPU
    void synchronizeState();   // Synchronizes GPU and CPU state
    void monitorPerformance(); // Monitors and reports performance

    // Member data
    std::vector<Ball> balls;
    SimConstants constants;
    mutable std::mutex ballsMutex;

    // Thread management
    std::atomic<bool> running{false};
    std::thread controlThread;
    std::thread computeThread;
    ThreadSync threadSync;
    FrameTiming timing;

    // GPU management
    GPUManager gpuManager;

    // Constants and configuration
    static constexpr float VELOCITY_RANGE = 100.0f;
    static constexpr float MIN_DISTANCE_FACTOR = 1.1f;
    static constexpr float PHYSICS_RATE = 240.0f;  // Physics updates per second
    static constexpr float DISPLAY_RATE = 30.0f;   // Display updates per second
    static constexpr float PHYSICS_DT = 1.0f / PHYSICS_RATE;
    static constexpr float DISPLAY_DT = 1.0f / DISPLAY_RATE;

    // Performance monitoring
    struct PerformanceMetrics {
        std::atomic<double> physicsTime{0.0};
        std::atomic<double> renderTime{0.0};
        std::atomic<int> activeThreads{0};
    } metrics;
};

} // namespace sim

#endif // BOUNCING_BALLS_SIMULATION_H
#ifndef BOUNCING_BALLS_TIMING_H
#define BOUNCING_BALLS_TIMING_H

#include <chrono>
#include <atomic>

namespace sim {

struct FrameTiming {
    std::chrono::steady_clock::time_point lastFrameTime;
    std::atomic<double> currentFps{0.0};

    double getFPS() const { return currentFps.load(); }
};

struct ThreadSync {
    std::atomic<bool> computationDone{true};

    void startComputation() { computationDone.store(false); }
    void endComputation() { computationDone.store(true); }
    void waitForComputation() {
        while (!computationDone.load()) {
            std::this_thread::yield();
        }
    }
};

} // namespace sim

#endif // BOUNCING_BALLS_TIMING_H
