#include "Simulation.h"
#include "Renderer.h"
#include <iostream>
#include <chrono>
#include <thread>

int main(int argc, char* argv[]) {
    int numBalls = 5; // Default number of balls
    if (argc > 1) {
        numBalls = std::clamp(std::stoi(argv[1]), 3, 10);
    }

    const int screenWidth = 1400;
    const int screenHeight = 900;

    Simulation simulation(numBalls, static_cast<float>(screenWidth), static_cast<float>(screenHeight));
    Renderer renderer(screenWidth, screenHeight);

    if (!renderer.initialize()) {
        std::cerr << "Failed to initialize renderer" << std::endl;
        return -1;
    }

    simulation.start();

    const float targetFrameTime = 1.0f / 30.0f; // 30 FPS

    while (!renderer.shouldClose()) {
        auto frameStartTime = std::chrono::high_resolution_clock::now();

        // Update simulation
        simulation.update();

        // Collect rendering data
        std::vector<std::tuple<float, float, float, int>> renderingData;
        simulation.getRenderingData(renderingData);

        // Render
        renderer.render(renderingData);

        // Maintain target frame rate
        auto frameDuration = std::chrono::duration<float>(std::chrono::high_resolution_clock::now() - frameStartTime).count();
        float sleepTime = targetFrameTime - frameDuration;
        if (sleepTime > 0.0f) {
            std::this_thread::sleep_for(std::chrono::duration<float>(sleepTime));
        }
    }

    simulation.stop();

    return 0;
}

