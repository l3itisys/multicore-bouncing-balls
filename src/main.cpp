#include "Simulation.h"
#include "Renderer.h"
#include <iostream>
#include <chrono>
#include <thread>
#include <tbb/parallel_for.h>

int main(int argc, char* argv[]) {
    const int screenWidth = 1400;
    const int screenHeight = 900;

    // Get number of balls from command line
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

        // Start simulation thread
        simulation.start();

        // Main loop
        const float targetFrameTime = 1.0f / 30.0f; // 30 FPS

        while (!renderer.shouldClose()) {
            auto frameStart = std::chrono::high_resolution_clock::now();

            // Get ball data for rendering
            const tbb::concurrent_vector<Ball*>& balls = simulation.getBalls();

            // Render
            renderer.render(balls);

            // Control frame rate
            auto frameEnd = std::chrono::high_resolution_clock::now();
            float frameTime = std::chrono::duration<float>(frameEnd - frameStart).count();
            if (frameTime < targetFrameTime) {
                std::this_thread::sleep_for(std::chrono::duration<float>(
                    targetFrameTime - frameTime));
            }
        }

        // Stop simulation
        simulation.stop();

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return -1;
    }

    return 0;
}

