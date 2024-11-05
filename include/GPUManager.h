#ifndef BOUNCING_BALLS_GPU_MANAGER_H
#define BOUNCING_BALLS_GPU_MANAGER_H

#define CL_HPP_ENABLE_EXCEPTIONS
#define CL_HPP_TARGET_OPENCL_VERSION 200
#include <CL/opencl.hpp>
#include "Types.h"
#include <vector>
#include <string>

namespace sim {

class GPUManager {
public:
    GPUManager() = default;
    ~GPUManager() = default;

    // Prevent copying
    GPUManager(const GPUManager&) = delete;
    GPUManager& operator=(const GPUManager&) = delete;

    // Initialization
    void initialize(size_t numBalls);

    // Simulation update
    void updateSimulation(std::vector<Ball>& balls, const SimConstants& constants);

private:
    // OpenCL initialization
    void createContext();
    void buildProgram();
    void createKernels();
    void createBuffers(size_t numBalls);

    // OpenCL objects
    cl::Context context;
    cl::CommandQueue queue;
    cl::Program program;
    cl::Kernel physicsKernel;
    cl::Kernel collisionKernel;

    // Device buffers
    cl::Buffer ballBuffer;
    cl::Buffer constantsBuffer;

    // State tracking
    size_t currentBufferSize = 0;
    bool initialized = false;

    // Constants
    static constexpr size_t WORKGROUP_SIZE = 256;
    static constexpr const char* KERNEL_FILENAME = "simulation.cl";
};

} // namespace sim

#endif // BOUNCING_BALLS_GPU_MANAGER_H

