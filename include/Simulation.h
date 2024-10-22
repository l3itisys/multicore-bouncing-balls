#ifndef SIMULATION_H
#define SIMULATION_H

#include "Ball.h"
#include <vector>
#include <memory>
#include <mutex>

class Simulation {
public:
    Simulation(int numBalls, float screenWidth, float screenHeight);
    ~Simulation();

    void update(float dt);
    const std::vector<std::unique_ptr<Ball>>& getBalls() const;

private:
    void assignBallsToGrid();
    void checkCollisionsInCell(int cellX, int cellY);
    void handleCollisionBetween(Ball& ballA, Ball& ballB);

    float screenWidth_, screenHeight_;
    std::vector<std::unique_ptr<Ball>> balls_;

    // Grid parameters for spatial partitioning
    float cellSize_;
    int gridWidth_;
    int gridHeight_;
    std::vector<std::vector<std::vector<Ball*>>> grid_; // 2D grid of cells containing pointers to balls
    std::vector<std::vector<std::unique_ptr<std::mutex>>> cellMutexes_;  // Mutexes for each cell
};

#endif // SIMULATION_H

