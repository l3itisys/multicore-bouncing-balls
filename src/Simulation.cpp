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
    if (!renderer.initialize(numBalls)) {
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

    // Initialize GPU manager
    gpuManager.initialize(numBalls, static_cast<int>(screenWidth),
                         static_cast<int>(screenHeight));
    gpuManager.setConstants(constants);

    // Initialize balls
    initializeBalls(numBalls);
}

Simulation::~Simulation() {
    stop();
}

void Simulation::initializeBalls(int numBalls) {
    std::mt19937 rng(std::random_device{}());
    std::uniform_real_distribution<float> distX(0.0f, screenWidth);
    std::uniform_real_distribution<float> distY(0.0f, screenHeight);
    std::uniform_real_distribution<float> distVel(-config::Balls::VELOCITY_RANGE, config::Balls::VELOCITY_RANGE);
    std::uniform_real_distribution<float> distRadius(config::Balls::MIN_RADIUS, config::Balls::MAX_RADIUS);
    std::uniform_int_distribution<size_t> distColor(0, config::Balls::COLOR_COUNT - 1);

    balls.resize(numBalls);
    for (auto& ball : balls) {
        ball.position = Vec2(distX(rng), distY(rng));
        ball.velocity = Vec2(distVel(rng), distVel(rng));
        ball.radius = distRadius(rng);
        ball.mass = ball.radius * ball.radius; // Mass proportional to area
        ball.color = config::Balls::COLORS[distColor(rng)];
        ball.padding = 0;
    }
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
            gpuManager.updatePhysics(balls);
        }

        nextUpdate += updateInterval;
        std::this_thread::sleep_until(nextUpdate);
    }
}

void Simulation::renderLoop() {
    using clock = std::chrono::steady_clock;
    auto lastTime = clock::now();
    auto nextFrame = clock::now();
    const auto frameInterval = std::chrono::duration_cast<clock::duration>(
        std::chrono::duration<double>(DISPLAY_DT)
    );

    int frameCount = 0;
    double fpsTimer = 0.0;
    double currentFPS = 0.0;

    std::cout << "Render loop starting" << std::endl;

    while (running && !shouldClose()) {
        auto currentTime = clock::now();
        double deltaTime = std::chrono::duration<double>(currentTime - lastTime).count();
        lastTime = currentTime;

        // Update FPS counter
        frameCount++;
        fpsTimer += deltaTime;
        if (fpsTimer >= 1.0) {
            currentFPS = frameCount / fpsTimer;
            frameCount = 0;
            fpsTimer = 0.0;
        }

        // Render current state
        if (!paused) {
            renderer.render(balls, currentFPS);
        }

        // Wait for next frame
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

