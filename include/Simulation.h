#ifndef SIMULATION_H
#define SIMULATION_H

#include "Ball.h"
#include <vector>
#include <memory>
#include <thread>
#include <atomic>

class Simulation {
public:
    Simulation(int numBalls, float screenWidth, float screenHeight);
    ~Simulation();

    void start();
    void stop();
    const std::vector<std::unique_ptr<Ball>>& getBalls() const;

private:
    void updateLoop();
    void update(float dt);

    std::atomic<bool> running_;
    float screenWidth_, screenHeight_;
    std::vector<std::unique_ptr<Ball>> balls_;
    std::thread simulationThread_;
};

#endif // SIMULATION_H

