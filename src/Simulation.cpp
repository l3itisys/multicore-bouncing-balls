#include "Simulation.h"
#include <random>
#include <iostream>
#include <iomanip>
#include <chrono>
#include <algorithm>

namespace sim {

// Use high_resolution_clock consistently
using clock = std::chrono::high_resolution_clock;
using duration = std::chrono::duration<double>;

Simulation::Simulation(int numBalls, float screenWidth, float screenHeight) {
    // Initialize simulation constants
    constants.dt = PHYSICS_DT;
    constants.gravity = 9.81f;
    constants.restitution = 0.8f;
    constants.screenDimensions = Vec2(screenWidth, screenHeight);

    try {
        // Initialize balls
        initializeBalls(numBalls);

        // Initialize GPU
        gpuManager.initialize(
            balls.size(),
            static_cast<int>(screenWidth),
            static_cast<int>(screenHeight)
        );

        std::cout << "Simulation initialized:\n"
                  << "- Number of balls: " << balls.size() << "\n"
                  << "- Physics rate: " << PHYSICS_RATE << " Hz\n"
                  << "- Display rate: " << DISPLAY_RATE << " Hz\n"
                  << "- Screen size: " << screenWidth << "x" << screenHeight << "\n";
    } catch (const std::exception& e) {
        std::cerr << "Initialization error: " << e.what() << std::endl;
        throw;
    }
}

Simulation::~Simulation() {
    stop();
}

void Simulation::start() {
    if (!running.exchange(true)) {
        // Initialize timing with high_resolution_clock
        timing.lastFrameTime = clock::now();

        // Start both threads
        controlThread = std::thread(&Simulation::controlThreadFunc, this);
        computeThread = std::thread(&Simulation::computationThreadFunc, this);

        std::cout << "Simulation threads started\n";
    }
}

void Simulation::stop() {
    running.store(false);

    if (controlThread.joinable()) {
        controlThread.join();
    }
    if (computeThread.joinable()) {
        computeThread.join();
    }
}

void Simulation::controlThreadFunc() {
    auto nextFrameTime = clock::now();
    const auto frameInterval = std::chrono::duration_cast<clock::duration>(
        duration(DISPLAY_DT)
    );

    int frameCount = 0;
    auto lastFpsUpdate = clock::now();

    std::cout << "Control thread started: Target " << DISPLAY_RATE << " FPS\n";

    while (running) {
        auto frameStart = clock::now();

        // Wait for computation to complete
        threadSync.waitForComputation();

        // Update display
        updateDisplay();

        // Calculate FPS
        frameCount++;
        auto now = clock::now();
        auto elapsed = now - lastFpsUpdate;

        if (std::chrono::duration_cast<duration>(elapsed).count() >= 1.0) {
            double fps = frameCount / std::chrono::duration_cast<duration>(elapsed).count();
            timing.currentFps.store(fps);

            // Print performance metrics
            std::cout << std::fixed << std::setprecision(1)
                      << "Display FPS: " << fps
                      << " | Physics time: " << metrics.physicsTime.load() << "ms"
                      << " | Render time: " << metrics.renderTime.load() << "ms"
                      << " | Active threads: " << metrics.activeThreads.load()
                      << std::endl;

            frameCount = 0;
            lastFpsUpdate = now;
        }

        // Control frame rate
        nextFrameTime += frameInterval;
        std::this_thread::sleep_until(nextFrameTime);
    }

    std::cout << "Control thread stopped\n";
}

void Simulation::computationThreadFunc() {
    auto nextUpdateTime = clock::now();
    const auto updateInterval = std::chrono::duration_cast<clock::duration>(
        duration(PHYSICS_DT)
    );

    metrics.activeThreads.fetch_add(1);
    std::cout << "Computation thread started: Physics rate " << PHYSICS_RATE << " Hz\n";

    while (running) {
        auto updateStart = clock::now();

        threadSync.startComputation();

        try {
            // Update physics on GPU
            updatePhysics();

            // Synchronize state
            synchronizeState();

            // Update metrics
            auto updateEnd = clock::now();
            metrics.physicsTime.store(
                std::chrono::duration_cast<duration>(updateEnd - updateStart).count() * 1000.0
            );

        } catch (const std::exception& e) {
            std::cerr << "Computation error: " << e.what() << std::endl;
            running.store(false);
            break;
        }

        threadSync.endComputation();

        // Control update rate
        nextUpdateTime += updateInterval;
        std::this_thread::sleep_until(nextUpdateTime);
    }

    metrics.activeThreads.fetch_sub(1);
    std::cout << "Computation thread stopped\n";
}

void Simulation::updatePhysics() {
    std::lock_guard<std::mutex> lock(ballsMutex);
    gpuManager.updateSimulation(balls, constants);
}

void Simulation::updateDisplay() {
    auto start = clock::now();

    try {
        std::lock_guard<std::mutex> lock(ballsMutex);
        gpuManager.updateDisplay();

    } catch (const std::exception& e) {
        std::cerr << "Display update error: " << e.what() << std::endl;
    }

    auto end = clock::now();
    metrics.renderTime.store(
        std::chrono::duration_cast<duration>(end - start).count() * 1000.0
    );
}

void Simulation::synchronizeState() {
    std::lock_guard<std::mutex> lock(ballsMutex);
    gpuManager.synchronizeState(balls);
}

void Simulation::initializeBalls(int numBalls) {
    std::random_device rd;
    std::mt19937 gen(rd());

    // Ball configurations
    struct BallConfig {
        float radius;
        float mass;
    };

    const std::array<BallConfig, 3> configs = {{
        {15.0f, 5.0f},   // Small
        {20.0f, 10.0f},  // Medium
        {25.0f, 15.0f}   // Large
    }};

    const std::array<uint32_t, 3> colors = {
        0xFF0000,  // Red
        0x00FF00,  // Green
        0x0000FF   // Blue
    };

    // Random distributions
    std::uniform_real_distribution<float> velDist(-VELOCITY_RANGE, VELOCITY_RANGE);
    std::uniform_int_distribution<size_t> configDist(0, configs.size() - 1);
    std::uniform_int_distribution<size_t> colorDist(0, colors.size() - 1);

    balls.reserve(numBalls);
    const int maxAttempts = 1000;

    for (int i = 0; i < numBalls; ++i) {
        Ball ball{};
        const auto& config = configs[configDist(gen)];

        ball.radius = config.radius;
        ball.mass = config.mass;
        ball.color = colors[colorDist(gen)];

        // Position distributions
        std::uniform_real_distribution<float> posX(
            ball.radius,
            constants.screenDimensions.x - ball.radius
        );
        std::uniform_real_distribution<float> posY(
            ball.radius,
            constants.screenDimensions.y - ball.radius
        );

        // Place ball without overlap
        bool validPosition = false;
        for (int attempt = 0; attempt < maxAttempts && !validPosition; ++attempt) {
            validPosition = true;
            ball.position = Vec2(posX(gen), posY(gen));

            for (const auto& existingBall : balls) {
                float dx = existingBall.position.x - ball.position.x;
                float dy = existingBall.position.y - ball.position.y;
                float minDist = (existingBall.radius + ball.radius) * 1.05f; // Reduced spacing between balls

                if (dx * dx + dy * dy < minDist * minDist) {
                    validPosition = false;
                    break;
                }
            }
        }

        if (!validPosition) {
            std::cerr << "Warning: Could not place ball " << i
                      << " after " << maxAttempts << " attempts.\n";
            continue;
        }

        // Set random velocity
        ball.velocity = Vec2(velDist(gen), velDist(gen));
        balls.push_back(ball);
    }

    if (balls.empty()) {
        throw std::runtime_error("Failed to place any balls");
    }

    std::cout << "Successfully initialized " << balls.size() << " balls\n";
}

std::vector<Ball> Simulation::getBalls() const {
    std::lock_guard<std::mutex> lock(ballsMutex);
    return balls;
}

} // namespace sim

