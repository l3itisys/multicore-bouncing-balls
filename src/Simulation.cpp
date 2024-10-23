#include "Simulation.h"
#include <random>
#include <chrono>
#include <tbb/parallel_for.h>
#include <iostream>

Simulation::Simulation(int numBalls, float screenWidth, float screenHeight)
    : screenWidth_(screenWidth), screenHeight_(screenHeight),
      grid_(screenWidth, screenHeight, 150.0f) {

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<float> vel(-200.0f, 200.0f); // Reduced velocities
    std::uniform_real_distribution<float> posX(0, screenWidth_);
    std::uniform_real_distribution<float> posY(0, screenHeight_);

    // Radius and mass pairs as per requirements
    std::vector<std::pair<float, float>> radiusMassOptions = {
        {50.0f, 5.0f},
        {100.0f, 10.0f},
        {150.0f, 15.0f}
    };

    // Primary colors as per requirements
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
            for (const auto& existingBall : balls_) {
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
                balls_.emplace_back(std::make_unique<Ball>(
                    i, radius, mass, x, y, vel(gen), vel(gen),
                    colors[colorDist(gen)], grid_, screenWidth_, screenHeight_
                ));
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
    // No threads to start in this version
}

void Simulation::stop() {
    // No threads to stop in this version
}

void Simulation::update(float dt) {
    const int subSteps = 2;
    float subDt = dt / subSteps;

    for (int step = 0; step < subSteps; ++step) {
        // Clear grid before updating positions
        grid_.clear();

        // Update positions in parallel
        tbb::parallel_for(size_t(0), balls_.size(), [&](size_t i) {
            balls_[i]->updatePosition(subDt);
        });

        // Insert balls into grid sequentially
        for (auto& ball : balls_) {
            grid_.insertBall(ball.get());
        }

        // Perform multiple collision resolution iterations
        const int collisionIterations = 5;
        for (int iter = 0; iter < collisionIterations; ++iter) {
            tbb::parallel_for(size_t(0), balls_.size(), [&](size_t i) {
                balls_[i]->detectCollisions();
            });
        }
    }
}

void Simulation::getRenderingData(std::vector<Ball*>& balls) {
    balls.clear();
    for (const auto& ball : balls_) {
        balls.push_back(ball.get());
    }
}

