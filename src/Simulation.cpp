#include "Simulation.h"
#include <random>
#include <chrono>
#include <tbb/parallel_for.h>
#include <iostream>

Simulation::Simulation(int numBalls, float screenWidth, float screenHeight)
    : screenWidth_(screenWidth), screenHeight_(screenHeight),
      grid_(screenWidth, screenHeight, 150.0f), running_(false), dt_(0.016f) {

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<float> vel(-100.0f, 100.0f);
    std::uniform_real_distribution<float> posX(0, screenWidth_);
    std::uniform_real_distribution<float> posY(0, screenHeight_);

    // Radius and mass pairs
    std::vector<std::pair<float, float>> radiusMassOptions = {
        {50.0f, 5.0f},
        {100.0f, 10.0f},
        {150.0f, 15.0f}
    };

    // Primary colors
    std::vector<int> colors = {
        0xFF0000,  // Red
        0x00FF00,  // Green
        0x0000FF   // Blue
    };

    std::uniform_int_distribution<size_t> optionDist(0, radiusMassOptions.size() - 1);
    std::uniform_int_distribution<size_t> colorDist(0, colors.size() - 1);

    int maxAttempts = 100; // Prevent infinite loops in case of too many overlaps
    for (int i = 0; i < numBalls; ++i) {
        bool placed = false;
        int attempts = 0;
        while (!placed && attempts < maxAttempts) {
            auto [radius, mass] = radiusMassOptions[optionDist(gen)];
            float x = std::clamp(posX(gen), radius, screenWidth_ - radius);
            float y = std::clamp(posY(gen), radius, screenHeight_ - radius);

            bool overlap = false;
            for (const auto& existingBall : ballsStorage_) {
                float ex, ey;
                existingBall->getPosition(ex, ey);
                float dx = ex - x;
                float dy = ey - y;
                float minDist = existingBall->getRadius() + radius;
                if (dx * dx + dy * dy < minDist * minDist) {
                    overlap = true;
                    break;
                }
            }

            if (!overlap) {
                auto ball = std::make_unique<Ball>(
                    i, radius, mass, x, y, vel(gen), vel(gen),
                    colors[colorDist(gen)], grid_, screenWidth_, screenHeight_
                );
                balls_.push_back(ball.get());        // Add raw pointer to concurrent_vector
                ballsStorage_.push_back(std::move(ball)); // Store unique_ptr
                placed = true;
            }
            ++attempts;
        }
        if (attempts == maxAttempts) {
            std::cerr << "Warning: Could not place all balls without overlap.\n";
            break;
        }
    }
}

Simulation::~Simulation() {
    stop();
}

void Simulation::start() {
    running_ = true;
    simulationThread_ = std::thread(&Simulation::simulationLoop, this);
}

void Simulation::stop() {
    running_ = false;
    if (simulationThread_.joinable()) {
        simulationThread_.join();
    }
}

void Simulation::simulationLoop() {
    while (running_) {
        auto startTime = std::chrono::high_resolution_clock::now();

        // Update simulation
        update(dt_);

        // Control simulation update rate
        auto endTime = std::chrono::high_resolution_clock::now();
        float frameTime = std::chrono::duration<float>(endTime - startTime).count();
        if (frameTime < dt_) {
            std::this_thread::sleep_for(std::chrono::duration<float>(dt_ - frameTime));
        }
    }
}

void Simulation::update(float dt) {
    // Apply gravity and update positions in parallel
    tbb::parallel_for(size_t(0), ballsStorage_.size(), [&](size_t i) {
        ballsStorage_[i]->applyGravity(dt);
        ballsStorage_[i]->updatePosition(dt);
        ballsStorage_[i]->checkBoundaryCollision();
    });

    // Clear grid
    grid_.clear();

    // Insert balls into grid
    for (auto& ball : ballsStorage_) {
        grid_.insertBall(ball.get());
    }

    // Detect and resolve collisions in parallel
    tbb::parallel_for(size_t(0), ballsStorage_.size(), [&](size_t i) {
        ballsStorage_[i]->detectCollisions();
    });
}

const tbb::concurrent_vector<Ball*>& Simulation::getBalls() const {
    return balls_;
}

