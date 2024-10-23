#ifndef SIMULATION_H
#define SIMULATION_H

#include "Ball.h"
#include "Grid.h"
#include <vector>
#include <memory>

class Simulation {
public:
    Simulation(int numBalls, float screenWidth, float screenHeight);
    ~Simulation();

    void start();
    void stop();
    void update(float dt);
    void getRenderingData(std::vector<Ball*>& balls);

private:
    float screenWidth_, screenHeight_;
    Grid grid_;
    std::vector<std::unique_ptr<Ball>> balls_;
};

#endif // SIMULATION_H

