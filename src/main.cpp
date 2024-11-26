#include "Simulation.h"
#include "Renderer.h"
#include "Config.h"
#include <iostream>
#include <stdexcept>
#include <thread>
#include <chrono>
#include <algorithm>
#include <iomanip>
#include <csignal>
#include <cstring>

// Global flag for graceful shutdown
volatile std::sig_atomic_t g_running = 1;

void signalHandler(int signum) {
    std::cout << "\nSignal (" << signum << ") received. Performing graceful shutdown..."
              << std::endl;
    g_running = 0;
}

void setupSignalHandling() {
    std::signal(SIGINT, signalHandler);
    std::signal(SIGTERM, signalHandler);
}

void printSystemInfo() {
    std::cout << "\n=== 2D Bouncing Balls Simulation (OpenCL) ===\n"
              << "Architecture:\n"
              << "- OpenCL for parallel computation\n"
              << "- OpenGL for rendering\n"
              << "- Double-buffered pipeline\n"
              << "- CPU-GPU work division\n"
              << "- Physics: " << sim::config::Physics::RATE << " Hz\n"
              << "- Display: " << sim::config::Display::TARGET_FPS << " FPS\n"
              << "======================================\n\n";
}

class SimulationApp {
public:
    SimulationApp(int numBalls, int width, int height)
        : simulation(numBalls, static_cast<float>(width), static_cast<float>(height))
        , renderer(width, height)
    {
        if (!renderer.initialize()) {
            throw std::runtime_error("Failed to initialize renderer");
        }
    }

    void run() {
        using clock = std::chrono::steady_clock;
        using duration = std::chrono::duration<double>;

        // Performance monitoring setup
        int frameCount = 0;
        auto lastReport = clock::now();
        auto simulationStart = clock::now();

        double totalFrameTime = 0.0;
        double maxFrameTime = 0.0;
        double minFrameTime = std::numeric_limits<double>::max();

        // Start simulation thread
        simulation.start();

        // Main loop (display thread)
        const duration frameTime(1.0 / sim::config::Display::TARGET_FPS);
        auto nextFrameTime = clock::now();

        std::cout << "Simulation started. Controls:\n"
                  << "  F - Toggle fullscreen\n"
                  << "  V - Toggle VSync\n"
                  << "  P - Pause/Resume\n"
                  << "  ESC - Exit\n\n";

        while (g_running && !renderer.shouldClose()) {
            auto frameStart = clock::now();

            // Get and render current frame
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
                // Get performance metrics
                sim::Renderer::PerformanceMetrics rendererMetrics;
                renderer.getPerformanceMetrics(rendererMetrics);
                const auto& simMetrics = simulation.getMetrics();

                printPerformanceMetrics(
                    frameCount / duration(now - lastReport).count(),
                    totalFrameTime / frameCount,
                    minFrameTime,
                    maxFrameTime,
                    rendererMetrics,
                    simMetrics
                );

                // Reset statistics
                frameCount = 0;
                totalFrameTime = 0.0;
                maxFrameTime = 0.0;
                minFrameTime = std::numeric_limits<double>::max();
                lastReport = now;
            }

            // Process input
            processInput();

            // Control frame rate
            nextFrameTime += std::chrono::duration_cast<clock::duration>(frameTime);
            std::this_thread::sleep_until(nextFrameTime);
        }

        // Cleanup
        simulation.stop();

        // Print final statistics
        auto simulationEnd = clock::now();
        double totalTime = duration(simulationEnd - simulationStart).count();

        std::cout << "\nSimulation completed:\n"
                  << "- Total runtime: " << std::fixed << std::setprecision(2)
                  << totalTime << " seconds\n"
                  << "- Average FPS: " << simulation.getCurrentFPS() << "\n"
                  << "- Frames dropped: " << simulation.getMetrics().frameDrops << "\n";
    }

private:
    void processInput() {
        if (glfwGetKey(renderer.getWindow(), GLFW_KEY_ESCAPE) == GLFW_PRESS) {
            g_running = 0;
        }

        // Handle fullscreen toggle
        static bool lastF = false;
        bool currentF = glfwGetKey(renderer.getWindow(), GLFW_KEY_F) == GLFW_PRESS;
        if (currentF && !lastF) {
            renderer.toggleFullscreen();
        }
        lastF = currentF;

        // Handle VSync toggle
        static bool lastV = false;
        bool currentV = glfwGetKey(renderer.getWindow(), GLFW_KEY_V) == GLFW_PRESS;
        if (currentV && !lastV) {
            renderer.toggleVSync();
        }
        lastV = currentV;

        // Handle pause toggle
        static bool lastP = false;
        bool currentP = glfwGetKey(renderer.getWindow(), GLFW_KEY_P) == GLFW_PRESS;
        if (currentP && !lastP) {
            if (simulation.isPaused()) {
                simulation.resume();
                std::cout << "Simulation resumed\n";
            } else {
                simulation.pause();
                std::cout << "Simulation paused\n";
            }
        }
        lastP = currentP;
    }

    void printPerformanceMetrics(
        double fps, double avgFrameTime, double minFrameTime, double maxFrameTime,
        const sim::Renderer::PerformanceMetrics& rendererMetrics,
        const sim::PerformanceMetrics& simMetrics) {

        // Clear previous line
        std::cout << "\r\033[K";  // ANSI escape codes to clear line

        // Performance color coding
        std::string fpsColor = (fps >= 30.0) ? "\033[32m" : "\033[31m";  // Green/Red
        std::string resetColor = "\033[0m";

        std::cout << std::fixed << std::setprecision(1)
                  << "Performance:"
                  << fpsColor << " FPS: " << fps << resetColor
                  << " | Frame Time (ms) - Avg: " << (avgFrameTime * 1000.0)
                  << " Min: " << (minFrameTime * 1000.0)
                  << " Max: " << (maxFrameTime * 1000.0)
                  << " | Physics: " << simMetrics.physicsTime << "ms"
                  << " | GPU Util: " << simMetrics.gpuUtilization << "%"
                  << " | Memory: " << (rendererMetrics.gpuMemoryUsage / (1024.0 * 1024.0))
                  << "MB"
                  << std::flush;

        // Log warnings if performance is poor
        if (fps < 30.0) {
            std::cerr << "\nWarning: Low framerate detected\n";
        }
        if (simMetrics.frameDrops > 0) {
            std::cerr << "\nWarning: " << simMetrics.frameDrops
                      << " frames dropped\n";
        }
    }

    sim::Simulation simulation;
    sim::Renderer renderer;
};

void printUsage(const char* programName) {
    std::cout << "Usage: " << programName << " [options]\n"
              << "Options:\n"
              << "  --balls N    Set number of balls (3-200, default: 50)\n"
              << "  --width N    Set window width (default: 1400)\n"
              << "  --height N   Set window height (default: 900)\n"
              << "  --help       Show this help message\n";
}

struct ApplicationConfig {
    int numBalls{sim::config::Balls::DEFAULT_COUNT};
    int width{sim::config::Display::DEFAULT_WIDTH};
    int height{sim::config::Display::DEFAULT_HEIGHT};
};

ApplicationConfig parseCommandLine(int argc, char* argv[]) {
    ApplicationConfig config;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];

        if (arg == "--help") {
            printUsage(argv[0]);
            exit(0);
        }
        else if (arg == "--balls" && i + 1 < argc) {
            try {
                config.numBalls = std::clamp(
                    std::stoi(argv[++i]),
                    sim::config::Balls::MIN_COUNT,
                    sim::config::Balls::MAX_COUNT
                );
            } catch (const std::exception& e) {
                std::cerr << "Invalid number of balls. Using default: "
                          << config.numBalls << "\n";
            }
        }
        else if (arg == "--width" && i + 1 < argc) {
            try {
                config.width = std::max(800, std::stoi(argv[++i]));
            } catch (const std::exception& e) {
                std::cerr << "Invalid width. Using default: "
                          << config.width << "\n";
            }
        }
        else if (arg == "--height" && i + 1 < argc) {
            try {
                config.height = std::max(600, std::stoi(argv[++i]));
            } catch (const std::exception& e) {
                std::cerr << "Invalid height. Using default: "
                          << config.height << "\n";
            }
        }
        else {
            std::cerr << "Unknown option: " << arg << "\n";
            printUsage(argv[0]);
            exit(1);
        }
    }

    return config;
}

int main(int argc, char* argv[]) {
    try {
        // Set up signal handling for graceful shutdown
        setupSignalHandling();

        // Parse command line arguments
        auto config = parseCommandLine(argc, argv);

        // Print system information
        printSystemInfo();
        std::cout << "Configuration:\n"
                  << "- Screen: " << config.width << "x" << config.height << "\n"
                  << "- Balls: " << config.numBalls << "\n"
                  << "- OpenGL for rendering\n"
                  << "- OpenCL for physics computation\n\n";

        // Initialize GLFW
        if (!glfwInit()) {
            throw std::runtime_error("Failed to initialize GLFW");
        }

        // Configure GLFW
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
        #ifdef __APPLE__
            glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
        #endif
        glfwWindowHint(GLFW_SAMPLES, 4); // Enable MSAA

        // Create window
        GLFWwindow* window = glfwCreateWindow(
            config.width, config.height,
            "2D Bouncing Balls Simulation",
            nullptr, nullptr
        );

        if (!window) {
            glfwTerminate();
            throw std::runtime_error("Failed to create GLFW window");
        }

        // Make OpenGL context current
        glfwMakeContextCurrent(window);

        // Initialize GLEW
        glewExperimental = GL_TRUE;
        GLenum err = glewInit();
        if (err != GLEW_OK) {
            glfwDestroyWindow(window);
            glfwTerminate();
            throw std::runtime_error(std::string("Failed to initialize GLEW: ") +
                                   (const char*)glewGetErrorString(err));
        }

        // Ensure OpenGL context is properly set up
        GLint glMajor, glMinor;
        glGetIntegerv(GL_MAJOR_VERSION, &glMajor);
        glGetIntegerv(GL_MINOR_VERSION, &glMinor);

        std::cout << "OpenGL Context Version: " << glMajor << "." << glMinor << std::endl;

        // Create and run simulation
        SimulationApp app(config.numBalls, config.width, config.height);
        app.run();

        // Cleanup
        glfwDestroyWindow(window);
        glfwTerminate();
        return 0;

    } catch (const cl::Error& e) {
        std::cerr << "OpenCL error: " << e.what() << " (" << e.err() << ")\n";
        glfwTerminate();
        return -1;
    } catch (const std::exception& e) {
        std::cerr << "Fatal error: " << e.what() << std::endl;
        glfwTerminate();
        return -1;
    }
}
