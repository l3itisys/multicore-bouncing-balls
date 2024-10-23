#include "Simulation.h"
#include "Renderer.h"
#include <iostream>
#include <chrono>
#include <thread>
#include <algorithm>

int main(int argc, char* argv[]) {
    const int screenWidth = 1400;
    const int screenHeight = 900;

    // Get number of balls from command line or use default
    int numBalls = 5;
    if (argc > 1) {
        try {
            numBalls = std::clamp(std::stoi(argv[1]), 3, 10);
        } catch (const std::exception& e) {
            std::cerr << "Invalid input. Using default 5 balls.\n";
        }
    }

    try {
        // Initialize simulation and renderer
        Simulation simulation(numBalls, static_cast<float>(screenWidth),
                            static_cast<float>(screenHeight));
        Renderer renderer(screenWidth, screenHeight);

        if (!renderer.initialize()) {
            std::cerr << "Failed to initialize renderer\n";
            return -1;
        }

        // Main loop
        const float targetFrameTime = 1.0f / 30.0f; // 30 FPS as per requirements
        const float dt = 1.0f / 60.0f; // Simulation timestep
        std::vector<Ball*> balls;

        while (!renderer.shouldClose()) {
            auto frameStart = std::chrono::high_resolution_clock::now();

            // Update simulation
            simulation.update(dt);

            // Get ball data for rendering
            simulation.getRenderingData(balls);

            // Render
            renderer.render(balls);

            // Maintain 30 FPS
            auto frameEnd = std::chrono::high_resolution_clock::now();
            float frameTime = std::chrono::duration<float>(frameEnd - frameStart).count();
            if (frameTime < targetFrameTime) {
                std::this_thread::sleep_for(std::chrono::duration<float>(
                    targetFrameTime - frameTime));
            }
        }

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return -1;
    }

    return 0;
}

