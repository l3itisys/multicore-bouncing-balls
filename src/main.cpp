#include "Simulation.h"
#include "Renderer.h"
#include <iostream>

int main() {
    const int numBalls = 10;
    const float screenWidth = 800.0f;
    const float screenHeight = 600.0f;

    Simulation simulation(numBalls, screenWidth, screenHeight);
    Renderer renderer(static_cast<int>(screenWidth), static_cast<int>(screenHeight));

    if (!renderer.initialize()) {
        std::cerr << "Failed to initialize renderer" << std::endl;
        return -1;
    }

    simulation.start();

    while (!renderer.shouldClose()) {
        renderer.render(simulation);
    }

    simulation.stop();

    return 0;
}

