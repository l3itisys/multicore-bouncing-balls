#ifndef BOUNCING_BALLS_SIMULATION_H
#define BOUNCING_BALLS_SIMULATION_H

#include "Types.h"
#include "Config.h"
#include "GPUManager.h"
#include "Renderer.h"
#include <iostream>
#include <vector>
#include <thread>
#include <memory>
#include <mutex>
#include <condition_variable>
#include <atomic>

namespace sim {

class Simulation {
public:
    // Constructor/Destructor
    Simulation(int numBalls, float screenWidth, float screenHeight);
    ~Simulation();

    // Prevent copying
    Simulation(const Simulation&) = delete;
    Simulation& operator=(const Simulation&) = delete;

    // Core functionality
    void start();
    void stop();
    void pause();
    void resume();
    bool isPaused() const { return paused.load(); }

    // State access
    std::vector<Ball> getBalls() const;
    double getCurrentFPS() const { return timing.getFPS(); }
    const PerformanceMetrics& getMetrics() const { return metrics; }

    // Configuration
    struct SimulationConfig {
        float gravity{config::Physics::GRAVITY};
        float restitution{config::Physics::RESTITUTION};
        float timeStep{config::Physics::DT};
        bool enableCollisions{true};
        bool enableGravity{true};
    };
    void setConfig(const SimulationConfig& config);
    SimulationConfig getConfig() const;

private:
    // Thread functions
    void controlThreadFunc();      // Display thread (30 FPS)
    void computationThreadFunc();  // Physics thread (240 Hz)
    void monitorThreadFunc();      // Performance monitoring thread

    // Initialization
    void initializeBalls(int numBalls);
    void validateConfig();
    void setupThreads();

    // Update functions
    void updateDisplay();
    void updatePhysics();
    void synchronizeState();
    void updatePerformanceMetrics();

    // Ball management
    struct BallManager {
        std::vector<Ball> generateInitialPositions(int numBalls, float width, float height);
        bool checkCollision(const Ball& a, const Ball& b);
        void resolveCollision(Ball& a, Ball& b, float restitution);
        Vec2 calculateInitialVelocity();
        uint32_t getRandomColor();
    } ballManager;

    // Pipeline management
    struct PipelineState {
        Frame currentFrame;
        Frame nextFrame;
        std::atomic<FrameState> state{FRAME_COMPUTING};
        mutable std::mutex frameMutex;
        std::condition_variable frameCondition;

        void waitForState(FrameState desiredState) {
            std::unique_lock<std::mutex> lock(frameMutex);
            frameCondition.wait(lock, [this, desiredState] {
                return state.load() == desiredState;
            });
        }

        void setState(FrameState newState) {
            state.store(newState);
            frameCondition.notify_all();
        }
    } pipeline;

    // Simulation data
    std::vector<Ball> balls;
    SimConstants constants;
    SimulationConfig config;

    // Thread management
    std::atomic<bool> running{false};
    std::atomic<bool> paused{false};
    std::thread controlThread;
    std::thread computeThread;
    std::thread monitorThread;

    // Synchronization
    mutable std::mutex ballsMutex;
    ThreadSync threadSync;
    FrameTiming timing;

    // Graphics management
    Renderer renderer;  // Must be declared before gpuManager
    GPUManager gpuManager;

    // Performance monitoring
    PerformanceMetrics metrics;
    struct MonitoringState {
        std::chrono::high_resolution_clock::time_point lastUpdate;
        std::vector<double> frameTimes;
        size_t frameTimeIndex{0};
        static constexpr size_t FRAME_TIME_HISTORY =
            config::Performance::FRAME_TIME_HISTORY;

        void recordFrameTime(double frameTime) {
            frameTimes[frameTimeIndex] = frameTime;
            frameTimeIndex = (frameTimeIndex + 1) % FRAME_TIME_HISTORY;
        }

        double getAverageFrameTime() const {
            double sum = 0.0;
            for (double time : frameTimes) {
                sum += time;
            }
            return sum / FRAME_TIME_HISTORY;
        }
    } monitoringState;

    // Scene dimensions
    float screenWidth;
    float screenHeight;

    // Constants from configuration
    static constexpr float MIN_DISTANCE_FACTOR = 1.1f;
    static constexpr float PHYSICS_RATE = config::Physics::RATE;
    static constexpr float DISPLAY_RATE = config::Display::TARGET_FPS;
    static constexpr float PHYSICS_DT = config::Physics::DT;
    static constexpr float DISPLAY_DT = 1.0f / DISPLAY_RATE;

    // Error handling
    void handleError(const std::exception& e, const char* location);
    void validateThreadState();
    void checkPerformance();

#ifdef _DEBUG
    // Debug utilities
    struct DebugState {
        std::atomic<uint64_t> physicsSteps{0};
        std::atomic<uint64_t> frameDrops{0};
        std::atomic<uint64_t> collisionChecks{0};

        struct Warning {
            std::string message;
            std::chrono::system_clock::time_point timestamp;
        };

        std::vector<Warning> warnings;
        std::mutex warningMutex;

        void addWarning(const std::string& message) {
            std::lock_guard<std::mutex> lock(warningMutex);
            warnings.push_back({
                message,
                std::chrono::system_clock::now()
            });

            // Keep only last 100 warnings
            if (warnings.size() > 100) {
                warnings.erase(warnings.begin());
            }
        }

        void clearWarnings() {
            std::lock_guard<std::mutex> lock(warningMutex);
            warnings.clear();
        }

        std::vector<Warning> getWarnings() {
            std::lock_guard<std::mutex> lock(warningMutex);
            return warnings;
        }
    } debugState;

    void validateSimulationState();
    void monitorResourceUsage();
    void checkThreadTiming();
    void logDebugInfo();
#endif

    // Helper functions
    void swapFrameBuffers() {
        std::lock_guard<std::mutex> lock(pipeline.frameMutex);
        std::swap(pipeline.currentFrame, pipeline.nextFrame);
    }

    bool checkFrameDropped() {
        auto now = std::chrono::high_resolution_clock::now();
        auto frameTime = std::chrono::duration<double>(
            now - timing.lastFrameTime).count();
        return frameTime > (1.5 / DISPLAY_RATE);
    }
};

} // namespace sim

#endif // BOUNCING_BALLS_SIMULATION_H
