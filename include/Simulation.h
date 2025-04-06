#ifndef SIMULATION_H
#define SIMULATION_H

#include "Ball.h"
#include "Grid.h"
#include <vector>
#include <memory>
#include <thread>
#include <atomic>
#include <tbb/concurrent_vector.h>

class Simulation {
public:
    Simulation(int numBalls, float screenWidth, float screenHeight);
    ~Simulation();

    void start();
    void stop();
    void update(float dt);
    const tbb::concurrent_vector<Ball*>& getBalls() const;

private:
    void simulationLoop();

    float screenWidth_, screenHeight_;
    Grid grid_;
    std::vector<std::unique_ptr<Ball>> ballsStorage_; // Unique ownership
    tbb::concurrent_vector<Ball*> balls_;             // Thread-safe access
    std::thread simulationThread_;
    std::atomic<bool> running_;
    float dt_;
};

#endif // SIMULATION_H

