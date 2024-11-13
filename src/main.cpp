#include "Simulation.h"
#include "Renderer.h"
#include <iostream>
#include <stdexcept>
#include <thread>
#include <chrono>
#include <algorithm>
#include <iomanip>
#include <csignal>

volatile std::sig_atomic_t g_running = 1;

void signalHandler(int signum) {
    std::cout << "\nSignal (" << signum << ") received. Performing graceful shutdown..." << std::endl;
    g_running = 0;
}

void printSystemInfo() {
    std::cout << "\n=== Bouncing Balls Simulation ===\n"
              << "Control Thread: Display updates at 30 FPS\n"
              << "Computation Thread: GPU-accelerated physics\n"
              << "Press Ctrl+C to exit\n"
              << "==============================\n\n";
}

void setupSignalHandling() {
    std::signal(SIGINT, signalHandler);
    std::signal(SIGTERM, signalHandler);
}

int main(int argc, char* argv[]) {
    try {
        setupSignalHandling();

        const int screenWidth = 1400;
        const int screenHeight = 900;

        // Get number of balls from command line
        int numBalls = 50;  // Default increased
        if (argc > 1) {
            try {
                numBalls = std::clamp(std::stoi(argv[1]), 3, 200);
            } catch (const std::exception& e) {
                std::cerr << "Invalid input. Using default " << numBalls << " balls.\n";
            }
        }

        // Print configuration
        printSystemInfo();
        std::cout << "Configuration:\n"
                  << "- Screen: " << screenWidth << "x" << screenHeight << "\n"
                  << "- Balls: " << numBalls << "\n"
                  << "- OpenGL for rendering\n"
                  << "- OpenCL for physics computation\n\n";

        // Create simulation and renderer
        sim::Simulation simulation(numBalls,
                                 static_cast<float>(screenWidth),
                                 static_cast<float>(screenHeight));

        sim::Renderer renderer(screenWidth, screenHeight);

        if (!renderer.initialize()) {
            throw std::runtime_error("Failed to initialize renderer");
        }

        // Performance monitoring setup
        using clock = std::chrono::steady_clock;
        using duration = std::chrono::duration<double>;

        int frameCount = 0;
        auto lastReport = clock::now();
        auto simulationStart = clock::now();

        double totalFrameTime = 0.0;
        double maxFrameTime = 0.0;
        double minFrameTime = std::numeric_limits<double>::max();

        // Start simulation thread (computation thread)
        simulation.start();

        // Main loop (control thread) - maintains 30 FPS
        const duration frameTime(1.0 / 30.0);
        auto nextFrameTime = clock::now();

        std::cout << "Simulation started. Press Ctrl+C to exit.\n\n";

        while (g_running && !renderer.shouldClose()) {
            auto frameStart = clock::now();

            // Get current state and render
            const auto& balls = simulation.getBalls();
            renderer.render(balls, simulation.getCurrentFPS());

            // Performance monitoring
            auto frameEnd = clock::now();
            auto frameDuration = duration(frameEnd - frameStart).count();

            totalFrameTime += frameDuration;
            maxFrameTime = std::max(maxFrameTime, frameDuration);
            minFrameTime = std::min(minFrameTime, frameDuration);
            frameCount++;

            // Print statistics every second
            auto now = clock::now();
            if (duration(now - lastReport).count() >= 1.0) {
                double avgFPS = frameCount / duration(now - lastReport).count();
                double avgFrameTime = totalFrameTime / frameCount;

                std::cout << std::fixed << std::setprecision(1)
                         << "FPS: " << avgFPS
                         << " | Frame time (ms) - Avg: " << avgFrameTime * 1000.0
                         << " Min: " << minFrameTime * 1000.0
                         << " Max: " << maxFrameTime * 1000.0
                         << std::endl;

                // Reset statistics
                frameCount = 0;
                totalFrameTime = 0.0;
                maxFrameTime = 0.0;
                minFrameTime = std::numeric_limits<double>::max();
                lastReport = now;
            }

            // Control frame rate
            nextFrameTime += std::chrono::duration_cast<clock::duration>(frameTime);
            std::this_thread::sleep_until(nextFrameTime);
        }

        // Clean up
        simulation.stop();

        // Print final statistics
        auto simulationEnd = clock::now();
        double totalTime = duration(simulationEnd - simulationStart).count();
        std::cout << "\nSimulation completed:\n"
                  << "- Total runtime: " << totalTime << " seconds\n"
                  << "- Average FPS: " << simulation.getCurrentFPS() << "\n";

        return 0;

    } catch (const std::exception& e) {
        std::cerr << "Fatal error: " << e.what() << std::endl;
        return -1;
    }
}
