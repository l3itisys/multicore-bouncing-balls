#include "Simulation.h"
#include "Renderer.h"
#include <iostream>
#include <stdexcept>
#include <thread>
#include <chrono>
#include <algorithm>
#include <iomanip>
#include <csignal>

#include <GLFW/glfw3.h>

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

        // Configuration
        const int screenWidth = 1400;
        const int screenHeight = 900;

        // Get number of balls from command line
        int numBalls = 5;  // Default
        if (argc > 1) {
            try {
                numBalls = std::clamp(std::stoi(argv[1]), 3, 1000);
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

        // Initialize GLFW
        if (!glfwInit()) {
            throw std::runtime_error("Failed to initialize GLFW");
        }

        // Configure GLFW window hints
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
        glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

        // Create window
        GLFWwindow* window = glfwCreateWindow(
            screenWidth, screenHeight,
            "Bouncing Balls Simulation (OpenCL)",
            nullptr, nullptr
        );

        if (!window) {
            glfwTerminate();
            throw std::runtime_error("Failed to create GLFW window");
        }

        glfwMakeContextCurrent(window);

        // Initialize GLEW
        glewExperimental = GL_TRUE;
        GLenum err = glewInit();
        if (err != GLEW_OK) {
            std::cerr << "GLEW initialization failed: " << glewGetErrorString(err) << std::endl;
            glfwDestroyWindow(window);
            glfwTerminate();
            throw std::runtime_error("Failed to initialize GLEW");
        }

        // Enable VSync
        glfwSwapInterval(1);

        // Now that OpenGL context is created and GLEW is initialized, we can initialize the Simulation
        sim::Simulation simulation(numBalls,
                                   static_cast<float>(screenWidth),
                                   static_cast<float>(screenHeight));

        // Create renderer, passing the window and GPUManager
        sim::Renderer renderer(screenWidth, screenHeight, simulation.getGPUManager());

        if (!renderer.initialize(window)) {
            simulation.stop();
            glfwDestroyWindow(window);
            glfwTerminate();
            throw std::runtime_error("Failed to initialize renderer");
        }

        // Start simulation thread
        simulation.start();

        // Main loop
        while (g_running && !renderer.shouldClose()) {
            // Update display
            renderer.render(simulation.getCurrentFPS());

            // Swap buffers and poll events
            glfwSwapBuffers(window);
            glfwPollEvents();
        }

        // Clean up
        simulation.stop();
        renderer.cleanup();
        glfwDestroyWindow(window);
        glfwTerminate();

        std::cout << "\nSimulation completed:\n"
                  << "- Average FPS: " << simulation.getCurrentFPS() << "\n";

        return 0;

    } catch (const std::exception& e) {
        std::cerr << "Fatal error: " << e.what() << std::endl;
        return -1;
    }
}

