// src/Simulation.cpp

#include "Simulation.h"
#include <random>
#include <iostream>
#include <iomanip>
#include <algorithm>
#include <cmath>
#include <sstream>

namespace sim {

Simulation::Simulation(int numBalls, float screenWidth_, float screenHeight_)
    : renderer(static_cast<int>(screenWidth_), static_cast<int>(screenHeight_))
    , screenWidth(screenWidth_)
    , screenHeight(screenHeight_)
{
    // Initialize simulation constants
    constants.dt = config::Physics::DT;
    constants.gravity = config::Physics::GRAVITY;
    constants.restitution = config::Physics::RESTITUTION;
    constants.screenDimensions = Vec2(screenWidth, screenHeight);

    try {
        // Initialize simulation components
        initializeBalls(numBalls);
        validateConfig();

        // Initialize double buffer frames
        pipeline.currentFrame.balls = balls;
        pipeline.nextFrame.balls = balls;

        // Initialize renderer first to create OpenGL context
        if (!renderer.initialize()) {
            throw std::runtime_error("Failed to initialize renderer");
        }

        // Now initialize GPU manager after OpenGL context is created
        gpuManager.initialize(
            balls.size(),
            static_cast<int>(screenWidth),
            static_cast<int>(screenHeight)
        );

        // Initialize monitoring
        monitoringState.frameTimes.resize(MonitoringState::FRAME_TIME_HISTORY);

        std::cout << "Simulation initialized with pipeline architecture:\n"
                  << "- Number of balls: " << balls.size() << "\n"
                  << "- Physics rate: " << PHYSICS_RATE << " Hz\n"
                  << "- Display rate: " << DISPLAY_RATE << " Hz\n"
                  << "- Screen size: " << screenWidth << "x" << screenHeight << "\n"
                  << "- Double buffering enabled\n";
    } catch (const std::exception& e) {
        handleError(e, "Initialization");
        throw;
    }
}

Simulation::~Simulation() {
    stop();
}

void Simulation::start() {
    if (!running.exchange(true)) {
        timing.lastFrameTime = std::chrono::high_resolution_clock::now();
        pipeline.state.store(FRAME_COMPUTING);
        try {
            setupThreads();
            std::cout << "Simulation pipeline started\n";
        } catch (const std::exception& e) {
            handleError(e, "Thread Setup");
            running = false;
            throw;
        }
    }
}

void Simulation::stop() {
    if (running.exchange(false)) {
        // Signal threads to stop
        pipeline.frameCondition.notify_all();

        // Wait for threads to complete
        if (computeThread.joinable()) computeThread.join();
        if (controlThread.joinable()) controlThread.join();
        if (monitorThread.joinable()) monitorThread.join();

        std::cout << "Simulation stopped\n";
    }
}

void Simulation::pause() {
    paused.store(true);
}

void Simulation::resume() {
    paused.store(false);
    pipeline.frameCondition.notify_all();
}

void Simulation::setupThreads() {
    // Start computation thread (physics at 240Hz)
    computeThread = std::thread(&Simulation::computationThreadFunc, this);

    // Start control thread (display at 30Hz)
    controlThread = std::thread(&Simulation::controlThreadFunc, this);

    // Start monitoring thread
    monitorThread = std::thread(&Simulation::monitorThreadFunc, this);

    // Set thread priorities if possible
    #ifdef _WIN32
    SetThreadPriority(computeThread.native_handle(), THREAD_PRIORITY_HIGHEST);
    SetThreadPriority(controlThread.native_handle(), THREAD_PRIORITY_ABOVE_NORMAL);
    SetThreadPriority(monitorThread.native_handle(), THREAD_PRIORITY_BELOW_NORMAL);
    #endif
}

void Simulation::controlThreadFunc() {
    using clock = std::chrono::high_resolution_clock;
    auto nextFrameTime = clock::now();
    const auto frameInterval = std::chrono::duration_cast<clock::duration>(
        std::chrono::duration<double>(DISPLAY_DT)
    );

    metrics.activeThreads.fetch_add(1);
    int frameCount = 0;
    auto lastFpsUpdate = clock::now();

    while (running) {
        auto frameStart = clock::now();

        // Wait for next frame computation if not paused
        {
            std::unique_lock<std::mutex> lock(pipeline.frameMutex);
            pipeline.frameCondition.wait(lock, [this] {
                return !running ||
                       (!paused && pipeline.state.load() == FRAME_READY);
            });
        }

        if (!running) break;

        try {
            // Swap frame buffers
            {
                std::lock_guard<std::mutex> lock(pipeline.frameMutex);
                std::swap(pipeline.currentFrame, pipeline.nextFrame);
                pipeline.state.store(FRAME_DISPLAYING);
            }

            // Signal compute thread
            pipeline.frameCondition.notify_one();

            // Update display
            updateDisplay();

            // Calculate FPS and update timing
            frameCount++;
            auto now = clock::now();
            auto elapsed = now - lastFpsUpdate;

            if (std::chrono::duration<double>(elapsed).count() >= 1.0) {
                double fps = frameCount / std::chrono::duration<double>(elapsed).count();
                timing.currentFps.store(fps);

                // Store frame time for monitoring
                monitoringState.frameTimes[monitoringState.frameTimeIndex++] =
                    std::chrono::duration<double>(elapsed).count() / frameCount;

                if (monitoringState.frameTimeIndex >= MonitoringState::FRAME_TIME_HISTORY) {
                    monitoringState.frameTimeIndex = 0;
                }

                frameCount = 0;
                lastFpsUpdate = now;
            }
        } catch (const std::exception& e) {
            handleError(e, "Display Update");
            continue;
        }

        // Control frame rate
        auto frameEnd = clock::now();
        auto frameTime = std::chrono::duration<double>(frameEnd - frameStart).count();
        metrics.renderTime.store(frameTime * 1000.0);  // Store in milliseconds

        nextFrameTime += frameInterval;
        std::this_thread::sleep_until(nextFrameTime);
    }

    metrics.activeThreads.fetch_sub(1);
}

void Simulation::computationThreadFunc() {
    using clock = std::chrono::high_resolution_clock;
    auto nextUpdateTime = clock::now();
    const auto updateInterval = std::chrono::duration_cast<clock::duration>(
        std::chrono::duration<double>(PHYSICS_DT)
    );

    metrics.activeThreads.fetch_add(1);

    while (running) {
        auto updateStart = clock::now();

        // Wait if paused
        if (paused) {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
            continue;
        }

        // Wait for display thread to finish using the buffers
        {
            std::unique_lock<std::mutex> lock(pipeline.frameMutex);
            pipeline.frameCondition.wait(lock, [this] {
                return !running || pipeline.state.load() != FRAME_SWAPPING;
            });
        }

        if (!running) break;

        try {
            // Update physics for next frame using GPU
            {
                std::lock_guard<std::mutex> lock(pipeline.frameMutex);
                gpuManager.updateSimulation(pipeline.nextFrame.balls, constants);
                pipeline.state.store(FRAME_READY);
            }

            // Signal display thread
            pipeline.frameCondition.notify_one();

            // Update metrics
            auto updateEnd = clock::now();
            metrics.physicsTime.store(
                std::chrono::duration<double>(updateEnd - updateStart).count() * 1000.0
            );

        } catch (const std::exception& e) {
            handleError(e, "Physics Update");
            continue;
        }

        // Control update rate
        nextUpdateTime += updateInterval;
        std::this_thread::sleep_until(nextUpdateTime);
    }

    metrics.activeThreads.fetch_sub(1);
}

void Simulation::monitorThreadFunc() {
    using clock = std::chrono::high_resolution_clock;
    const auto monitorInterval = std::chrono::seconds(1);
    auto nextUpdateTime = clock::now();

    while (running) {
        try {
            updatePerformanceMetrics();
            checkPerformance();

            #ifdef _DEBUG
            validateSimulationState();
            monitorResourceUsage();
            checkThreadTiming();
            #endif

        } catch (const std::exception& e) {
            handleError(e, "Performance Monitoring");
        }

        nextUpdateTime += monitorInterval;
        std::this_thread::sleep_until(nextUpdateTime);
    }
}

void Simulation::updateDisplay() {
    auto start = std::chrono::high_resolution_clock::now();

    try {
        gpuManager.updateDisplay();
    } catch (const std::exception& e) {
        handleError(e, "Display Update");
        throw;
    }

    auto end = std::chrono::high_resolution_clock::now();
    metrics.renderTime.store(
        std::chrono::duration<double>(end - start).count() * 1000.0
    );
}

// Initialize balls with proper spatial distribution
void Simulation::initializeBalls(int numBalls) {
    if (numBalls < config::Balls::MIN_COUNT || numBalls > config::Balls::MAX_COUNT) {
        throw std::runtime_error("Invalid number of balls specified");
    }

    std::random_device rd;
    std::mt19937 gen(rd());

    // Ball configurations
    struct BallConfig {
        float radius;
        float mass;
        uint32_t color;
    };

    const std::array<BallConfig, 3> configs = {{
        {15.0f, 3.0f, config::Balls::COLORS[0]},  // Small
        {20.0f, 5.0f, config::Balls::COLORS[1]},  // Medium
        {25.0f, 7.0f, config::Balls::COLORS[2]}   // Large
    }};

    // Random distributions
    std::uniform_real_distribution<float> velDist(-config::Balls::VELOCITY_RANGE,
                                                 config::Balls::VELOCITY_RANGE);
    std::uniform_int_distribution<size_t> configDist(0, configs.size() - 1);

    balls.reserve(numBalls);

    // Grid-based placement to avoid overlaps
    const int gridCols = static_cast<int>(std::sqrt(numBalls * screenWidth / screenHeight));
    const int gridRows = (numBalls + gridCols - 1) / gridCols;
    const float cellWidth = screenWidth / gridCols;
    const float cellHeight = screenHeight / gridRows;

    int ballsPlaced = 0;
    const int maxAttempts = 50;

    for (int row = 0; row < gridRows && ballsPlaced < numBalls; ++row) {
        for (int col = 0; col < gridCols && ballsPlaced < numBalls; ++col) {
            const auto& config = configs[configDist(gen)];
            Ball ball{};
            ball.radius = config.radius;
            ball.mass = config.mass;
            ball.color = config.color;

            // Try to place ball in current grid cell
            bool validPosition = false;
            for (int attempt = 0; attempt < maxAttempts && !validPosition; ++attempt) {
                // Calculate position within cell, leaving margin for radius
                float minX = col * cellWidth + ball.radius;
                float maxX = (col + 1) * cellWidth - ball.radius;
                float minY = row * cellHeight + ball.radius;
                float maxY = (row + 1) * cellHeight - ball.radius;

                std::uniform_real_distribution<float> xDist(minX, maxX);
                std::uniform_real_distribution<float> yDist(minY, maxY);

                ball.position = Vec2(xDist(gen), yDist(gen));
                ball.velocity = Vec2(velDist(gen), velDist(gen));

                // Check for overlaps with existing balls
                validPosition = true;
                for (const auto& existingBall : balls) {
                    float minDist = (existingBall.radius + ball.radius) *
                                   config::Balls::MIN_SEPARATION;
                    float dx = existingBall.position.x - ball.position.x;
                    float dy = existingBall.position.y - ball.position.y;
                    if (dx * dx + dy * dy < minDist * minDist) {
                        validPosition = false;
                        break;
                    }
                }
            }

            if (validPosition) {
                balls.push_back(ball);
                ballsPlaced++;
            }
        }
    }

    if (balls.empty()) {
        throw std::runtime_error("Failed to initialize any balls");
    }

    std::cout << "Successfully initialized " << balls.size() << " balls\n";
}

void Simulation::updatePerformanceMetrics() {
    auto& m = metrics;

    // Calculate average frame time
    double avgFrameTime = 0.0;
    for (double ft : monitoringState.frameTimes) {
        avgFrameTime += ft;
    }
    avgFrameTime /= MonitoringState::FRAME_TIME_HISTORY;

    // Update metrics
    m.frameDrops.store(
        static_cast<int>(avgFrameTime > (1.0 / DISPLAY_RATE * 1.2))
    );

    // Get GPU metrics
    auto gpuStats = gpuManager.getStats();
    m.gpuUtilization.store(gpuStats.computeTime / (1.0 / PHYSICS_RATE) * 100.0);
}

std::vector<Ball> Simulation::getBalls() const {
    std::lock_guard<std::mutex> lock(pipeline.frameMutex);
    return pipeline.currentFrame.balls;
}

void Simulation::handleError(const std::exception& e, const char* location) {
    std::cerr << "Error in " << location << ": " << e.what() << std::endl;
    #ifdef _DEBUG
    debugState.addWarning(std::string(location) + ": " + e.what());
    #endif
}

void Simulation::checkPerformance() {
    // Check FPS
    double currentFPS = timing.getFPS();
    if (currentFPS < DISPLAY_RATE * 0.9) {  // Below 90% of target
        std::cerr << "Performance warning: Low FPS (" << currentFPS << ")\n";
    }

    // Check computation time
    double physicsTime = metrics.physicsTime.load();
    if (physicsTime > PHYSICS_DT * 1000.0) {  // Taking longer than frame time
        std::cerr << "Performance warning: Physics computation exceeding frame time\n";
    }

    // Check GPU utilization
    double gpuUtil = metrics.gpuUtilization.load();
    if (gpuUtil > 90.0) {  // Above 90% utilization
        std::cerr << "Performance warning: High GPU utilization (" << gpuUtil << "%)\n";
    }
}

void Simulation::validateConfig() {
    // Validate screen dimensions
    if (screenWidth <= 0 || screenHeight <= 0) {
        throw std::runtime_error("Invalid screen dimensions");
    }

    // Validate physics parameters
    if (constants.gravity < -100.0f || constants.gravity > 100.0f) {
        throw std::runtime_error("Invalid gravity value");
    }
    if (constants.restitution < 0.0f || constants.restitution > 1.0f) {
        throw std::runtime_error("Invalid restitution value");
    }
    if (constants.dt <= 0.0f || constants.dt > 0.1f) {
        throw std::runtime_error("Invalid time step value");
    }

    // Validate frame rates
    if (PHYSICS_RATE < DISPLAY_RATE) {
        throw std::runtime_error("Physics rate must be higher than display rate");
    }
}

void Simulation::setConfig(const SimulationConfig& newConfig) {
    std::lock_guard<std::mutex> lock(ballsMutex);
    config = newConfig;
    constants.gravity = config.gravity;
    constants.restitution = config.restitution;
    constants.dt = config.timeStep;
}

Simulation::SimulationConfig Simulation::getConfig() const {
    std::lock_guard<std::mutex> lock(ballsMutex);
    return config;
}

#ifdef _DEBUG
void Simulation::validateSimulationState() {
    // Check for invalid ball positions
    for (const auto& ball : pipeline.currentFrame.balls) {
        if (std::isnan(ball.position.x) || std::isnan(ball.position.y)) {
            debugState.addWarning("Invalid ball position detected");
        }
        if (std::isnan(ball.velocity.x) || std::isnan(ball.velocity.y)) {
            debugState.addWarning("Invalid ball velocity detected");
        }
    }

    // Check thread states
    validateThreadState();
}

void Simulation::validateThreadState() {
    if (metrics.activeThreads.load() != 3) {  // Should have 3 active threads
        debugState.addWarning("Incorrect number of active threads");
    }
}

void Simulation::monitorResourceUsage() {
    // Monitor GPU memory usage
    auto gpuStats = gpuManager.getStats();
    if (gpuStats.gpuMemoryUsage > 0) {
        std::cout << "GPU Memory Usage: "
                  << (gpuStats.gpuMemoryUsage / (1024.0 * 1024.0))
                  << " MB\n";
    }

    // Monitor CPU memory usage
    size_t totalMemory =
        sizeof(Ball) * balls.size() * 2 +  // Double buffered balls
        sizeof(SimConstants) +              // Constants
        sizeof(PerformanceMetrics) +        // Metrics
        monitoringState.frameTimes.capacity() * sizeof(double); // Frame times

    std::cout << "CPU Memory Usage: "
              << (totalMemory / 1024.0)
              << " KB\n";
}

void Simulation::checkThreadTiming() {
    // Check physics timing
    double physicsTime = metrics.physicsTime.load();
    if (physicsTime > (PHYSICS_DT * 1000.0 * 0.9)) {  // Using 90% of frame time
        debugState.addWarning("Physics computation approaching frame time limit");
    }

    // Check rendering timing
    double renderTime = metrics.renderTime.load();
    if (renderTime > (DISPLAY_DT * 1000.0 * 0.9)) {  // Using 90% of frame time
        debugState.addWarning("Rendering approaching frame time limit");
    }

    // Check frame drops
    if (metrics.frameDrops.load() > 0) {
        debugState.addWarning("Frames are being dropped");
    }
}

void Simulation::DebugState::addWarning(const std::string& warning) {
    std::lock_guard<std::mutex> lock(warningMutex);
    warnings.push_back(warning);

    // Keep only last 100 warnings
    if (warnings.size() > 100) {
        warnings.erase(warnings.begin());
    }

    // Print warning to console
    std::cerr << "Warning: " << warning << std::endl;
}

std::vector<std::string> Simulation::DebugState::getWarnings() {
    std::lock_guard<std::mutex> lock(warningMutex);
    return warnings;
}

void Simulation::DebugState::clearWarnings() {
    std::lock_guard<std::mutex> lock(warningMutex);
    warnings.clear();
}
#endif

} // namespace sim
