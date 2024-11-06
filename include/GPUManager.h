#ifndef BOUNCING_BALLS_GPU_MANAGER_H
#define BOUNCING_BALLS_GPU_MANAGER_H

#define CL_HPP_ENABLE_EXCEPTIONS
#define CL_HPP_TARGET_OPENCL_VERSION 200
#include <CL/opencl.hpp>
#include "Types.h"
#include <vector>
#include <string>
#include <memory>

namespace sim {

class GPUManager {
public:
    GPUManager() = default;
    ~GPUManager() = default;

    GPUManager(const GPUManager&) = delete;
    GPUManager& operator=(const GPUManager&) = delete;

    void initialize(size_t numBalls, int screenWidth, int screenHeight);

    void updateSimulation(std::vector<Ball>& balls, const SimConstants& constants);
    void updateDisplay();
    void synchronizeState(std::vector<Ball>& balls);

private:
    // OpenCL initialization
    void createContext();
    void buildProgram();
    void createKernels();
    void createBuffers(size_t numBalls, int screenWidth, int screenHeight);

    cl::Platform selectPlatform();
    cl::Device selectDevice(cl::Platform& platform);
    std::string loadKernelSource();
    void printDeviceInfo();

    struct {
        int width;
        int height;
    } screenDimensions;

    // OpenCL objects
    cl::Context context;
    cl::CommandQueue queue;
    cl::Program program;

    // Kernels
    cl::Kernel updatePositionsKernel;
    cl::Kernel pixelUpdateKernel;
    cl::Kernel collisionKernel;

    // Device buffers
    cl::Buffer ballBuffer;
    cl::Buffer pixelBuffer;
    cl::Buffer constantsBuffer;

    size_t currentBufferSize = 0;
    bool initialized = false;

    // Constants
    static constexpr size_t WORKGROUP_SIZE = 256;  // Optimal for Intel GPUs
    static constexpr const char* KERNEL_FILENAME = "simulation.cl";

    void checkError(cl_int error, const char* message);
};

}

#endif // BOUNCING_BALLS_GPU_MANAGER_H
