#ifndef SIMULATION_H
#define SIMULATION_H

#include "Ball.h"
#include "Grid.h"
#include <vector>
#include <memory>
#include <thread>
#include <atomic>
#include <mutex>
#include <condition_variable>

class Simulation {
public:
    Simulation(int numBalls, float screenWidth, float screenHeight);
    ~Simulation();

    void start();
    void stop();
    void update(float dt);
    void getRenderingData(std::vector<Ball*>& balls);

private:
    void simulationLoop();

    float screenWidth_, screenHeight_;
    Grid grid_;
    std::vector<std::unique_ptr<Ball>> balls_;
    std::thread simulationThread_;
    std::atomic<bool> running_;
    std::mutex dataMutex_;
    std::condition_variable cv_;
    float dt_;
    bool dataReady_;
};

#endif // SIMULATION_H

