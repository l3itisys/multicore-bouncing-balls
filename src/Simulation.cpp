#include "Simulation.h"
#include <random>
#include <iostream>
#include <chrono>
#include <algorithm>
#include <cmath>
#include <mutex>
#include <tbb/parallel_for.h>
#include <tbb/blocked_range.h>

Simulation::Simulation(int numBalls, float screenWidth, float screenHeight)
    : screenWidth_(screenWidth), screenHeight_(screenHeight) {
    std::random_device rd;
    std::mt19937 gen(rd());

    std::uniform_real_distribution<float> posX(0, screenWidth);
    std::uniform_real_distribution<float> posY(0, screenHeight);
    std::uniform_real_distribution<float> vel(-100, 100);
    std::uniform_int_distribution<int> colorDist(0x0000FF, 0xFF0000);

    // Define possible radii and corresponding masses
    std::vector<std::pair<float, float>> radiusMassOptions = {
        {50.0f, 5.0f},
        {100.0f, 10.0f},
        {150.0f, 15.0f}
    };

    std::uniform_int_distribution<size_t> optionDist(0, radiusMassOptions.size() - 1);

    for (int i = 0; i < numBalls; ++i) {
        size_t optionIndex = optionDist(gen);
        float r = radiusMassOptions[optionIndex].first;
        float mass = radiusMassOptions[optionIndex].second;
        float x = std::clamp(posX(gen), r, screenWidth - r);
        float y = std::clamp(posY(gen), r, screenHeight - r);
        float vx = vel(gen);
        float vy = vel(gen);
        int color = colorDist(gen);

        balls_.emplace_back(std::make_unique<Ball>(i, r, mass, x, y, vx, vy, color));
    }

    // Initialize grid parameters
    cellSize_ = 150.0f; // Max radius * 2 to ensure balls fit within a cell
    gridWidth_ = static_cast<int>(std::ceil(screenWidth_ / cellSize_));
    gridHeight_ = static_cast<int>(std::ceil(screenHeight_ / cellSize_));

    // Initialize grid_
    grid_.resize(gridHeight_, std::vector<std::vector<Ball*>>(gridWidth_));

    // Initialize cellMutexes_
    cellMutexes_.resize(gridHeight_);
    for (int y = 0; y < gridHeight_; ++y) {
        cellMutexes_[y].resize(gridWidth_);
        for (int x = 0; x < gridWidth_; ++x) {
            cellMutexes_[y][x] = std::make_unique<std::mutex>();
        }
    }
}

Simulation::~Simulation() {
    // Destructor
}

void Simulation::update(float dt) {
    // Apply gravity and update positions in parallel
    tbb::parallel_for(tbb::blocked_range<size_t>(0, balls_.size()), [&](const tbb::blocked_range<size_t>& range) {
        for (size_t i = range.begin(); i != range.end(); ++i) {
            auto& ball = balls_[i];
            ball->applyGravity(dt);
            ball->updatePosition(dt);
            ball->checkBoundaryCollision(screenWidth_, screenHeight_);
        }
    });

    // Assign balls to grid cells
    assignBallsToGrid();

    // Collision detection and resolution
    tbb::parallel_for(tbb::blocked_range<size_t>(0, static_cast<size_t>(gridHeight_)), [&](const tbb::blocked_range<size_t>& r) {
        for (size_t y = r.begin(); y != r.end(); ++y) {
            for (int x = 0; x < gridWidth_; ++x) {
                checkCollisionsInCell(x, static_cast<int>(y));
            }
        }
    });
}

void Simulation::assignBallsToGrid() {
    // Clear the grid in parallel
    tbb::parallel_for(tbb::blocked_range<size_t>(0, static_cast<size_t>(gridHeight_)), [&](const tbb::blocked_range<size_t>& r) {
        for (size_t y = r.begin(); y != r.end(); ++y) {
            for (int x = 0; x < gridWidth_; ++x) {
                grid_[y][x].clear();
            }
        }
    });

    // Assign balls to cells in parallel
    tbb::parallel_for(tbb::blocked_range<size_t>(0, balls_.size()), [&](const tbb::blocked_range<size_t>& r) {
        for (size_t i = r.begin(); i != r.end(); ++i) {
            auto& ball = balls_[i];
            int cellX = static_cast<int>(ball->getX() / cellSize_);
            int cellY = static_cast<int>(ball->getY() / cellSize_);

            // Clamp cell indices
            cellX = std::clamp(cellX, 0, gridWidth_ - 1);
            cellY = std::clamp(cellY, 0, gridHeight_ - 1);

            // Lock the cell before modifying
            {
                std::lock_guard<std::mutex> lock(*cellMutexes_[cellY][cellX]);
                grid_[cellY][cellX].push_back(ball.get());
            }
        }
    });
}

void Simulation::checkCollisionsInCell(int cellX, int cellY) {
    std::vector<Ball*>& cellBalls = grid_[cellY][cellX];

    // Check collisions within the same cell
    for (size_t i = 0; i < cellBalls.size(); ++i) {
        for (size_t j = i + 1; j < cellBalls.size(); ++j) {
            handleCollisionBetween(*cellBalls[i], *cellBalls[j]);
        }
    }

    // Check collisions with neighboring cells
    for (int offsetY = -1; offsetY <= 1; ++offsetY) {
        for (int offsetX = -1; offsetX <= 1; ++offsetX) {
            int neighborX = cellX + offsetX;
            int neighborY = cellY + offsetY;

            // Skip if neighbor is out of bounds or the same cell
            if (neighborX < 0 || neighborX >= gridWidth_ || neighborY < 0 || neighborY >= gridHeight_)
                continue;
            if (neighborX == cellX && neighborY == cellY)
                continue;

            std::vector<Ball*>& neighborBalls = grid_[neighborY][neighborX];

            for (auto* ballA : cellBalls) {
                for (auto* ballB : neighborBalls) {
                    handleCollisionBetween(*ballA, *ballB);
                }
            }
        }
    }
}

void Simulation::handleCollisionBetween(Ball& ballA, Ball& ballB) {
    // Early check without locking
    float dx = ballB.getX() - ballA.getX();
    float dy = ballB.getY() - ballA.getY();
    float distanceSquared = dx * dx + dy * dy;
    float minDist = ballA.getRadius() + ballB.getRadius();

    if (distanceSquared < minDist * minDist) {
        // Lock both balls using std::scoped_lock to prevent data races
        std::scoped_lock lock(ballA.getMutex(), ballB.getMutex());

        // Recalculate after locking to ensure accuracy
        dx = ballB.getX() - ballA.getX();
        dy = ballB.getY() - ballA.getY();
        distanceSquared = dx * dx + dy * dy;

        if (distanceSquared < minDist * minDist) {
            // Perform collision handling
            ballA.handleCollision(ballB);
        }
    }
}

const std::vector<std::unique_ptr<Ball>>& Simulation::getBalls() const {
    return balls_;
}

