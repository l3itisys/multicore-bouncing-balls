#ifndef BOUNCING_BALLS_GPU_MANAGER_H
#define BOUNCING_BALLS_GPU_MANAGER_H

#define CL_HPP_ENABLE_EXCEPTIONS
#define CL_HPP_TARGET_OPENCL_VERSION 200
#include <CL/opencl.hpp>
#include "Types.h"
#include <vector>
#include <string>
#include <memory>
#include <GL/glew.h>
#include <GLFW/glfw3.h>

// OpenGL headers
#include <GL/glew.h>

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
    void updateDisplay();
    void synchronizeState(std::vector<Ball>& balls);

    // Accessor for OpenGL texture
    GLuint getTextureID() const { return glTextureID; }

private:
    // OpenCL initialization helpers
    void createContext();
    void buildProgram();
    void createKernels();
    void createBuffers(size_t numBalls, int screenWidth, int screenHeight);

    // OpenCL resource management
    cl::Platform selectPlatform();
    cl::Device selectDevice(cl::Platform& platform);
    std::string loadKernelSource();
    void printDeviceInfo();

    // OpenGL-OpenCL interop helpers
    void createGLTexture(int width, int height);

    // Screen dimensions
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
    cl::ImageGL pixelImage;   // OpenCL image linked with OpenGL texture
    cl::Buffer constantsBuffer;

    // OpenGL texture
    GLuint glTextureID = 0;

    // Buffer state tracking
    size_t currentBufferSize = 0;
    bool initialized = false;

    // Constants
    static constexpr size_t WORKGROUP_SIZE = 256;  // Optimal for Intel GPUs
    static constexpr const char* KERNEL_FILENAME = "simulation.cl";

    // Error checking helper
    void checkError(cl_int error, const char* message);
};

} // namespace sim

#endif // BOUNCING_BALLS_GPU_MANAGER_H

