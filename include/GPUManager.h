#ifndef BOUNCING_BALLS_GPU_MANAGER_H
#define BOUNCING_BALLS_GPU_MANAGER_H

#include "Types.h"
#include "Config.h"
#include <vector>
#include <string>

namespace sim {

class GPUManager {
public:
    GPUManager() = default;
    ~GPUManager();

    // Core functionality
    void initialize(size_t numBalls, int screenWidth, int screenHeight);
    void cleanup();
    void updatePhysics(std::vector<Ball>& balls);
    void setConstants(const SimConstants& consts) { constants = consts; }

private:
    // Initialization helpers
    void createContext();
    void buildProgram();
    void createKernels();
    void createBuffers(size_t numBalls);
    std::string loadKernelSource();

    // OpenCL objects
    cl::Context context;
    cl::CommandQueue queue;
    cl::Program program;
    cl::Device device;

    // Kernels
    cl::Kernel physicsKernel;
    cl::Kernel collisionKernel;

    // Buffers
    cl::Buffer ballBuffer;
    cl::Buffer constantsBuffer;

    // State
    bool initialized{false};
    size_t numBalls{0};
    size_t workGroupSize{256};
    SimConstants constants;

    struct {
        int width{0};
        int height{0};
    } screen;
};

} // namespace sim

#endif // BOUNCING_BALLS_GPU_MANAGER_H
