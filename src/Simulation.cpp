#include "Simulation.h"
#include <random>
#include <iostream>
#include <chrono>
#include <algorithm>
#include <cmath> // Include this for std::isfinite

Simulation::Simulation(int numBalls, float screenWidth, float screenHeight)
    : running_(false), screenWidth_(screenWidth), screenHeight_(screenHeight) {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<float> posX(0, screenWidth);
    std::uniform_real_distribution<float> posY(0, screenHeight);
    std::uniform_real_distribution<float> vel(-50, 50);
    std::uniform_real_distribution<float> radiusDist(20, 40);

    for (int i = 0; i < numBalls; ++i) {
        float r = radiusDist(gen);
        float x = std::clamp(posX(gen), r, screenWidth - r);
        float y = std::clamp(posY(gen), r, screenHeight - r);
        float vx = vel(gen);
        float vy = vel(gen);
        float mass = r * r * 3.14159f; // Mass proportional to area
        int color = std::uniform_int_distribution<>(0x0000FF, 0xFF0000)(gen);

        balls_.emplace_back(std::make_unique<Ball>(i, r, mass, x, y, vx, vy, color));
    }
}

Simulation::~Simulation() {
    stop();
}

void Simulation::start() {
    running_ = true;
    simulationThread_ = std::thread(&Simulation::updateLoop, this);
}

void Simulation::stop() {
    running_ = false;
    if (simulationThread_.joinable()) {
        simulationThread_.join();
    }
}

const std::vector<std::unique_ptr<Ball>>& Simulation::getBalls() const {
    return balls_;
}

void Simulation::updateLoop() {
    auto previousTime = std::chrono::high_resolution_clock::now();
    const float targetFrameTime = 1.0f / 30.0f; // 60 FPS

    while (running_) {
        auto currentTime = std::chrono::high_resolution_clock::now();
        float dt = std::chrono::duration<float>(currentTime - previousTime).count();
        previousTime = currentTime;

        update(dt);

        // Sleep to maintain target frame rate
        auto frameDuration = std::chrono::high_resolution_clock::now() - currentTime;
        auto sleepTime = std::chrono::duration<float>(targetFrameTime - frameDuration.count());
        if (sleepTime.count() > 0) {
            std::this_thread::sleep_for(std::chrono::duration<float>(sleepTime));
        }
    }
}

void Simulation::update(float dt) {
    // Apply gravity and update positions
    for (auto& ball : balls_) {
        ball->applyGravity(dt);
        ball->updatePosition(dt);
        ball->checkBoundaryCollision(screenWidth_, screenHeight_);

        // Check for NaN or Infinity
        if (!std::isfinite(ball->getX()) || !std::isfinite(ball->getY()) ||
            !std::isfinite(ball->getVx()) || !std::isfinite(ball->getVy())) {
            std::cerr << "Invalid state detected in ball " << ball->getId() << std::endl;
        }
    }

    // Collision detection and resolution
    for (size_t i = 0; i < balls_.size(); ++i) {
        for (size_t j = i + 1; j < balls_.size(); ++j) {
            balls_[i]->handleCollision(*balls_[j]);
        }
    }

    // Optional: Log ball states for debugging
     for (const auto& ball : balls_) {
         ball->debugLog();
     }
}

