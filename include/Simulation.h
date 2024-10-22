#ifndef SIMULATION_H
#define SIMULATION_H

#include "Ball.h"
#include <vector>
#include <memory>

class Simulation {
public:
    Simulation(int numBalls, float screenWidth, float screenHeight);
    ~Simulation();

    void start();
    void stop();

    void update();

    void getRenderingData(std::vector<std::tuple<float, float, float, int>>& renderingData);

private:
    float screenWidth_, screenHeight_;
    std::vector<std::unique_ptr<Ball>> balls_;
    std::vector<Ball*> ballPointers_; // For collision detection
};

#endif // SIMULATION_H

