#include "Simulation.h"
#include <iostream>
#include <stdexcept>
#include <csignal>

namespace {
    volatile std::sig_atomic_t g_running = 1;
}

void signalHandler(int /*signum*/) {
    g_running = 0;
}

void setupSignalHandling() {
    std::signal(SIGINT, signalHandler);
    std::signal(SIGTERM, signalHandler);
}

int main(int argc, char* argv[]) {
    try {
        setupSignalHandling();

        int numBalls = sim::config::Balls::DEFAULT_COUNT;
        if (argc > 1) {
            numBalls = std::stoi(argv[1]);
            numBalls = std::clamp(numBalls, sim::config::Balls::MIN_COUNT, sim::config::Balls::MAX_COUNT);
        }

        sim::Simulation simulation(
            numBalls,
            sim::config::Display::DEFAULT_WIDTH,
            sim::config::Display::DEFAULT_HEIGHT
        );

        std::cout << "\nBouncing Balls Simulation\n"
                  << "Controls:\n"
                  << "  ESC - Exit\n"
                  << "  P   - Pause/Resume\n\n";

        simulation.start();

        while (g_running && !simulation.shouldClose()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }

        simulation.stop();
        return 0;
    }
    catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
}

