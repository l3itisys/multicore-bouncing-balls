#include "Simulation.h"
#include <random>
#include <algorithm>
#include <tuple>

Simulation::Simulation(int numBalls, float screenWidth, float screenHeight)
    : screenWidth_(screenWidth), screenHeight_(screenHeight) {
    std::random_device rd;
    std::mt19937 gen(rd());

    std::uniform_real_distribution<float> posX(0, screenWidth);
    std::uniform_real_distribution<float> posY(0, screenHeight);
    std::uniform_real_distribution<float> vel(-100, 100);
    std::uniform_int_distribution<int> colorDist(0, 2);

    // Define possible radii and masses
    std::vector<std::pair<float, float>> radiusMassOptions = {
        {50.0f, 5.0f},
        {100.0f, 10.0f},
        {150.0f, 15.0f}
    };

    std::uniform_int_distribution<size_t> optionDist(0, radiusMassOptions.size() - 1);

    // Primary colors: red, green, blue
    std::vector<int> colors = {
        0xFF0000, // Red
        0x00FF00, // Green
        0x0000FF  // Blue
    };

    for (int i = 0; i < numBalls; ++i) {
        size_t optionIndex = optionDist(gen);
        float r = radiusMassOptions[optionIndex].first;
        float mass = radiusMassOptions[optionIndex].second;
        float x = std::clamp(posX(gen), r, screenWidth - r);
        float y = std::clamp(posY(gen), r, screenHeight - r);
        float vx = vel(gen);
        float vy = vel(gen);
        int color = colors[colorDist(gen)];

        balls_.emplace_back(std::make_unique<Ball>(i, r, mass, x, y, vx, vy, color));
    }

    // Prepare pointers for collision detection
    for (auto& ball : balls_) {
        ballPointers_.push_back(ball.get());
    }

    // Assign allBalls_ pointer for collision detection
    for (auto& ball : balls_) {
        ball->allBalls_ = &ballPointers_;
    }
}

Simulation::~Simulation() {
    stop();
}

void Simulation::start() {
    for (auto& ball : balls_) {
        ball->start();
    }
}

void Simulation::stop() {
    for (auto& ball : balls_) {
        ball->stop();
    }
}

void Simulation::update() {
    // Signal all balls to proceed
    for (auto& ball : balls_) {
        ball->signalUpdate();
    }

    // Wait for all balls to complete their update
    for (auto& ball : balls_) {
        ball->waitForUpdate();
    }
}

void Simulation::getRenderingData(std::vector<std::tuple<float, float, float, int>>& renderingData) {
    renderingData.clear();
    for (auto& ball : balls_) {
        float x, y, radius;
        int color;
        ball->getRenderingData(x, y, radius, color);
        renderingData.emplace_back(x, y, radius, color);
    }
}

