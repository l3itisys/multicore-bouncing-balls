#include "Simulation.h"
#include <random>
#include <iostream>
#include <chrono>

namespace sim {

Simulation::Simulation(int numBalls, float screenWidth_, float screenHeight_)
    : renderer(screenWidth_, screenHeight_)
    , screenWidth(screenWidth_)
    , screenHeight(screenHeight_)
{
    std::cout << "Creating simulation with " << numBalls << " balls" << std::endl;

    // Initialize renderer first
    if (!renderer.initialize()) {
        throw std::runtime_error("Failed to initialize renderer");
    }

    // Store this pointer in window for callbacks
    glfwSetWindowUserPointer(renderer.getWindow(), this);

    // Set up keyboard callback
    glfwSetKeyCallback(renderer.getWindow(), keyCallback);

    // Initialize simulation constants
    constants.dt = PHYSICS_DT;
    constants.gravity = config::Physics::GRAVITY;
    constants.restitution = config::Physics::RESTITUTION;
    constants.screenDimensions = Vec2(screenWidth, screenHeight);

    std::cout << "Initialized constants:" << std::endl
              << "  dt: " << constants.dt << std::endl
              << "  gravity: " << constants.gravity << std::endl
              << "  restitution: " << constants.restitution << std::endl
              << "  screen: " << screenWidth << "x" << screenHeight << std::endl;

    // Initialize balls
    initializeBalls(numBalls);

    // Initialize GPU manager
    gpuManager.initialize(balls.size(), static_cast<int>(screenWidth),
                         static_cast<int>(screenHeight));
    gpuManager.setConstants(constants);
}

Simulation::~Simulation() {
    stop();
}

void Simulation::initializeBalls(int numBalls) {
    std::random_device rd;
    std::mt19937 gen(rd());

    std::uniform_real_distribution<float> radiusDist(
        config::Balls::MIN_RADIUS,
        config::Balls::MAX_RADIUS
    );

    std::uniform_real_distribution<float> velDist(
        -config::Balls::VELOCITY_RANGE,
        config::Balls::VELOCITY_RANGE
    );

    balls.reserve(numBalls);

    // Calculate grid for initial placement
    int gridCols = static_cast<int>(std::sqrt(numBalls * screenWidth / screenHeight));
    int gridRows = (numBalls + gridCols - 1) / gridCols;
    float cellWidth = screenWidth / gridCols;
    float cellHeight = screenHeight / gridRows;

    for (int i = 0; i < numBalls; i++) {
        int row = i / gridCols;
        int col = i % gridCols;

        Ball ball;
        ball.radius = radiusDist(gen);
        ball.mass = ball.radius * ball.radius;
        ball.color = config::Balls::COLORS[i % config::Balls::COLOR_COUNT];

        // Ensure balls are placed within screen boundaries
        float margin = ball.radius * 2.0f;

        // Calculate position within grid cell
        float minX = col * cellWidth + margin;
        float maxX = (col + 1) * cellWidth - margin;
        float minY = row * cellHeight + margin;
        float maxY = (row + 1) * cellHeight - margin;

        // Ensure valid position ranges
        minX = std::max(minX, ball.radius);
        maxX = std::min(maxX, screenWidth - ball.radius);
        minY = std::max(minY, ball.radius);
        maxY = std::min(maxY, screenHeight - ball.radius);

        // Generate random position within valid range
        std::uniform_real_distribution<float> xDist(minX, maxX);
        std::uniform_real_distribution<float> yDist(minY, maxY);

        ball.position.x = xDist(gen);
        ball.position.y = yDist(gen);
        ball.velocity.x = velDist(gen);
        ball.velocity.y = velDist(gen);

        balls.push_back(ball);
    }

    std::cout << "Initialized " << balls.size() << " balls" << std::endl;
}

void Simulation::start() {
    if (!running.exchange(true)) {
        physicsThread = std::thread(&Simulation::physicsLoop, this);
        renderThread = std::thread(&Simulation::renderLoop, this);
    }
}

void Simulation::stop() {
    if (running.exchange(false)) {
        if (physicsThread.joinable()) physicsThread.join();
        if (renderThread.joinable()) renderThread.join();
    }
}

void Simulation::physicsLoop() {
    using clock = std::chrono::steady_clock;
    auto nextUpdate = clock::now();
    const auto updateInterval = std::chrono::duration_cast<clock::duration>(
        std::chrono::duration<double>(PHYSICS_DT)
    );

    while (running && !shouldClose()) {
        if (!paused) {
            {
                std::lock_guard<std::mutex> lock(ballsMutex);
                gpuManager.updatePhysics(balls);
            }
        }

        nextUpdate += updateInterval;
        std::this_thread::sleep_until(nextUpdate);
    }
}

void Simulation::renderLoop() {
    using clock = std::chrono::steady_clock;
    auto nextFrame = clock::now();
    const auto frameInterval = std::chrono::duration_cast<clock::duration>(
        std::chrono::duration<double>(DISPLAY_DT)
    );

    std::cout << "Render loop starting with " << balls.size() << " balls" << std::endl;

    while (running && !shouldClose()) {
        // Always process events
        glfwPollEvents();

        if (!paused) {
            std::vector<Ball> currentBalls;
            {
                std::lock_guard<std::mutex> lock(ballsMutex);
                currentBalls = balls;
            }

            renderer.render(currentBalls);
        }

        nextFrame += frameInterval;
        std::this_thread::sleep_until(nextFrame);
    }
}

void Simulation::keyCallback(GLFWwindow* window, int key, int /*scancode*/, int action, int /*mods*/) {
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
        glfwSetWindowShouldClose(window, GLFW_TRUE);
        return;
    }

    // Get simulation instance from window user pointer
    auto* sim = static_cast<Simulation*>(glfwGetWindowUserPointer(window));
    if (!sim) return;

    if (key == GLFW_KEY_P && action == GLFW_PRESS) {
        if (sim->isPaused()) {
            sim->resume();
        } else {
            sim->pause();
        }
    }
}

} // namespace sim
