#include "Simulation.h"
#include "Renderer.h"
#include <iostream>
#include <chrono>
#include <thread>

int main() {
    const int numBalls = 10;
    const float screenWidth = 1400.0f;
    const float screenHeight = 900.0f;

    Simulation simulation(numBalls, screenWidth, screenHeight);
    Renderer renderer(static_cast<int>(screenWidth), static_cast<int>(screenHeight));

    if (!renderer.initialize()) {
        std::cerr << "Failed to initialize renderer" << std::endl;
        return -1;
    }

    // Simulation loop variables
    auto previousTime = std::chrono::high_resolution_clock::now();
    const float targetFrameTime = 1.0f / 30.0f; // 30 FPS

    while (!renderer.shouldClose()) {
        auto currentTime = std::chrono::high_resolution_clock::now();
        float dt = std::chrono::duration<float>(currentTime - previousTime).count();
        previousTime = currentTime;

        simulation.update(dt);      // Control thread for the update
        renderer.render(simulation); // Control thread handles rendering

        // Sleep to maintain target frame rate
        auto frameDuration = std::chrono::high_resolution_clock::now() - currentTime;
        auto sleepTime = std::chrono::duration<float>(targetFrameTime - frameDuration.count());
        if (sleepTime.count() > 0) {
            std::this_thread::sleep_for(sleepTime);
        }
    }

    return 0;
}

