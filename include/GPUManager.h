#ifndef BOUNCING_BALLS_GPU_MANAGER_H
#define BOUNCING_BALLS_GPU_MANAGER_H

#include <GL/glew.h>
#include <GLFW/glfw3.h>

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
    ~GPUManager();

    // Prevent copying
    GPUManager(const GPUManager&) = delete;
    GPUManager& operator=(const GPUManager&) = delete;

    // Initialization
    void initialize(size_t numBalls, int screenWidth, int screenHeight);

    // Simulation update
    void updateSimulation(std::vector<Ball>& balls, const SimConstants& constants);

    // Display update
    void updateDisplay();
    void synchronizeState(std::vector<Ball>& balls);
    GLuint getTextureID() const { return glTextureID; }

private:
    // Helper functions
    void printDeviceInfo();
    cl::Platform selectPlatform();
    cl::Device selectDevice(cl::Platform& platform);
    std::string loadKernelSource();
    void createGLTexture(int width, int height);
    void checkError(cl_int error, const char* message);

    // OpenCL initialization
    void createContext();
    void buildProgram();
    void createKernels();
    void createBuffers(size_t numBalls, int screenWidth, int screenHeight);

    // OpenCL objects
    cl::Context context;
    cl::CommandQueue queue;
    cl::Program program;
    cl::Kernel updatePositionsKernel;
    cl::Kernel pixelUpdateKernel;
    cl::Kernel collisionKernel;

    // Device buffers and textures
    cl::Buffer ballBuffer;
    cl::Buffer constantsBuffer;
    cl::ImageGL pixelImage;
    GLuint glTextureID = 0;

    // Screen dimensions
    struct {
        int width;
        int height;
    } screenDimensions;

    // State tracking
    size_t currentBufferSize = 0;
    bool initialized = false;

    // Constants
    static constexpr size_t WORKGROUP_SIZE = 512; // Increased for better parallelism
    static constexpr size_t THREADS_PER_BALL = 64; // Multiple threads per ball
    static constexpr const char* KERNEL_FILENAME = "simulation.cl";
};

} // namespace sim

#endif // BOUNCING_BALLS_GPU_MANAGER_H

