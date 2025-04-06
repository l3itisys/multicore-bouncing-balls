#ifndef BOUNCING_BALLS_SIMULATION_H
#define BOUNCING_BALLS_SIMULATION_H

#include "Types.h"
#include "Config.h"
#include "GPUManager.h"
#include "Renderer.h"
#include <vector>
#include <thread>
#include <atomic>
#include <mutex>

namespace sim {

class Simulation {
public:
    Simulation(int numBalls, float screenWidth, float screenHeight);
    ~Simulation();

    void start();
    void stop();
    void pause() { paused = true; }
    void resume() { paused = false; }
    bool isPaused() const { return paused; }
    bool shouldClose() const { return renderer.shouldClose(); }

private:
    void initializeBalls(int numBalls);
    void physicsLoop();
    void renderLoop();
    static void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);

    // Core components
    SimConstants constants;
    GPUManager gpuManager;
    Renderer renderer;

    // Thread management
    std::atomic<bool> running{false};
    std::atomic<bool> paused{false};
    std::thread physicsThread;
    std::thread renderThread;

    // Scene dimensions
    float screenWidth;
    float screenHeight;

    // Balls data
    std::vector<Ball> balls;

    // Constants
    static constexpr float PHYSICS_RATE = config::Physics::RATE;
    static constexpr float DISPLAY_RATE = config::Display::TARGET_FPS;
    static constexpr float PHYSICS_DT = 1.0f / PHYSICS_RATE;
    static constexpr float DISPLAY_DT = 1.0f / DISPLAY_RATE;
};

} // namespace sim

#endif // BOUNCING_BALLS_SIMULATION_H

