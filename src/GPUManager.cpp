#include "GPUManager.h"
#include <fstream>
#include <iostream>
#include <cmath>
#include <filesystem>
#include <sstream>

namespace sim {

void GPUManager::initialize(size_t numBalls, int screenWidth, int screenHeight) {
    if (initialized && numBalls <= currentBufferSize) {
        return;
    }

    try {
        createContext();
        buildProgram();
        createKernels();
        createBuffers(numBalls, screenWidth, screenHeight);
        initialized = true;
        currentBufferSize = numBalls;

        screenDimensions.width = screenWidth;
        screenDimensions.height = screenHeight;

        printDeviceInfo();

    } catch (const cl::Error& e) {
        std::cerr << "OpenCL initialization error: "
                  << e.what() << " (" << e.err() << ")" << std::endl;
        throw;
    }
}

void GPUManager::printDeviceInfo() {
    auto device = context.getInfo<CL_CONTEXT_DEVICES>()[0];
    std::cout << "\nOpenCL Device Information:\n"
              << "  Device: " << device.getInfo<CL_DEVICE_NAME>() << "\n"
              << "  Vendor: " << device.getInfo<CL_DEVICE_VENDOR>() << "\n"
              << "  Max Compute Units: " << device.getInfo<CL_DEVICE_MAX_COMPUTE_UNITS>() << "\n"
              << "  Max Work Group Size: " << device.getInfo<CL_DEVICE_MAX_WORK_GROUP_SIZE>() << "\n"
              << "  Global Memory: " << device.getInfo<CL_DEVICE_GLOBAL_MEM_SIZE>() / (1024*1024) << " MB\n"
              << std::endl;
}

cl::Platform GPUManager::selectPlatform() {
    std::vector<cl::Platform> platforms;
    cl::Platform::get(&platforms);

    // try to find Intel platform
    for (const auto& platform : platforms) {
        std::string vendor = platform.getInfo<CL_PLATFORM_VENDOR>();
        if (vendor.find("Intel") != std::string::npos) {
            return platform;
        }
    }

    // If no Intel platform, take the first available
    if (!platforms.empty()) {
        return platforms[0];
    }

    throw std::runtime_error("No OpenCL platform found");
}

cl::Device GPUManager::selectDevice(cl::Platform& platform) {
    std::vector<cl::Device> devices;

    try {
        // First try to get GPU devices
        platform.getDevices(CL_DEVICE_TYPE_GPU, &devices);
    } catch (const cl::Error&) {
        // If no GPU, try any available device
        platform.getDevices(CL_DEVICE_TYPE_ALL, &devices);
    }

    if (devices.empty()) {
        throw std::runtime_error("No OpenCL device found");
    }

    return devices[0];
}

void GPUManager::createContext() {
    cl::Platform platform = selectPlatform();
    cl::Device device = selectDevice(platform);

    std::cout << "Using OpenCL device: "
              << device.getInfo<CL_DEVICE_NAME>()
              << " from "
              << platform.getInfo<CL_PLATFORM_VENDOR>()
              << std::endl;

    context = cl::Context(device);
    queue = cl::CommandQueue(context, device, CL_QUEUE_PROFILING_ENABLE);
}

std::string GPUManager::loadKernelSource() {
    std::filesystem::path kernelPath = KERNEL_FILENAME;

    // Try current directory and build directory
    if (!std::filesystem::exists(kernelPath)) {
        kernelPath = std::filesystem::current_path() / KERNEL_FILENAME;
    }

    std::ifstream file(kernelPath);
    if (!file.is_open()) {
        throw std::runtime_error(
            "Could not open kernel file: " + kernelPath.string()
        );
    }

    return std::string(
        std::istreambuf_iterator<char>(file),
        std::istreambuf_iterator<char>()
    );
}

void GPUManager::buildProgram() {
    std::string source = loadKernelSource();

    // Create program source properly
    cl::Program::Sources sources;
    sources.push_back({source.c_str(), source.length()});

    try {
        program = cl::Program(context, sources);

        std::string options = "-cl-std=CL2.0 "
                            "-cl-mad-enable "         // Enable multiply-add optimization
                            "-cl-fast-relaxed-math "  // Faster math operations
                            "-cl-no-signed-zeros";    // Ignore sign of zero

        program.build(options.c_str());

    } catch (const cl::Error& e) {
        std::cerr << "OpenCL build error:\n"
                  << program.getBuildInfo<CL_PROGRAM_BUILD_LOG>(
                         context.getInfo<CL_CONTEXT_DEVICES>()[0]
                     )
                  << std::endl;
        throw;
    }
}

void GPUManager::createKernels() {
    updatePositionsKernel = cl::Kernel(program, "updateBallPhysics");
    pixelUpdateKernel = cl::Kernel(program, "updateBallPixels");
    collisionKernel = cl::Kernel(program, "detectCollisions");
}

void GPUManager::createBuffers(size_t numBalls, int screenWidth, int screenHeight) {
    // Calculate buffer sizes
    size_t ballBufferSize = sizeof(Ball) * numBalls;
    size_t pixelBufferSize = sizeof(cl_uchar4) * screenWidth * screenHeight;

    // Create buffers with proper memory flags
    ballBuffer = cl::Buffer(
        context,
        CL_MEM_READ_WRITE | CL_MEM_ALLOC_HOST_PTR,
        ballBufferSize
    );

    pixelBuffer = cl::Buffer(
        context,
        CL_MEM_WRITE_ONLY | CL_MEM_ALLOC_HOST_PTR,
        pixelBufferSize
    );

    constantsBuffer = cl::Buffer(
        context,
        CL_MEM_READ_ONLY | CL_MEM_ALLOC_HOST_PTR,
        sizeof(SimConstants)
    );
}

void GPUManager::updateSimulation(std::vector<Ball>& balls, const SimConstants& constants) {
    try {
        // Write data to device (non-blocking)
        queue.enqueueWriteBuffer(
            ballBuffer,
            CL_FALSE,
            0,
            sizeof(Ball) * balls.size(),
            balls.data()
        );

        queue.enqueueWriteBuffer(
            constantsBuffer,
            CL_FALSE,
            0,
            sizeof(SimConstants),
            &constants
        );

        // Update physics - one thread per ball
        updatePositionsKernel.setArg(0, ballBuffer);
        updatePositionsKernel.setArg(1, constantsBuffer);
        updatePositionsKernel.setArg(2, static_cast<int>(balls.size()));

        queue.enqueueNDRangeKernel(
            updatePositionsKernel,
            cl::NullRange,
            cl::NDRange(balls.size()),
            cl::NDRange(std::min(balls.size(), WORKGROUP_SIZE))
        );

        // Update pixels - many threads for parallel rendering
        pixelUpdateKernel.setArg(0, ballBuffer);
        pixelUpdateKernel.setArg(1, pixelBuffer);
        pixelUpdateKernel.setArg(2, constantsBuffer);
        pixelUpdateKernel.setArg(3, screenDimensions.width);
        pixelUpdateKernel.setArg(4, screenDimensions.height);
        pixelUpdateKernel.setArg(5, static_cast<int>(balls.size()));

        // Use many threads for pixel updates (screenWidth * screenHeight / 16)
        size_t numPixelThreads = (screenDimensions.width * screenDimensions.height) / 16;
        queue.enqueueNDRangeKernel(
            pixelUpdateKernel,
            cl::NullRange,
            cl::NDRange(numPixelThreads),
            cl::NDRange(WORKGROUP_SIZE)
        );

        // Detect and resolve collisions - multiple threads
        collisionKernel.setArg(0, ballBuffer);
        collisionKernel.setArg(1, constantsBuffer);
        collisionKernel.setArg(2, static_cast<int>(balls.size()));

        size_t numCollisionThreads = (balls.size() * (balls.size() - 1)) / 2;
        queue.enqueueNDRangeKernel(
            collisionKernel,
            cl::NullRange,
            cl::NDRange(numCollisionThreads),
            cl::NDRange(std::min(numCollisionThreads, WORKGROUP_SIZE))
        );

        // Read back results (blocking)
        queue.enqueueReadBuffer(
            ballBuffer,
            CL_TRUE,
            0,
            sizeof(Ball) * balls.size(),
            balls.data()
        );

    } catch (const cl::Error& e) {
        std::cerr << "OpenCL error in updateSimulation: "
                  << e.what() << " (" << e.err() << ")" << std::endl;
        throw;
    }
}

void GPUManager::updateDisplay() {
}

void GPUManager::synchronizeState(std::vector<Ball>& balls) {
    try {
        queue.enqueueReadBuffer(
            ballBuffer,
            CL_TRUE,
            0,
            sizeof(Ball) * balls.size(),
            balls.data()
        );
    } catch (const cl::Error& e) {
        std::cerr << "Error synchronizing state: "
                  << e.what() << " (" << e.err() << ")" << std::endl;
        throw;
    }
}

void GPUManager::checkError(cl_int error, const char* message) {
    if (error != CL_SUCCESS) {
        throw std::runtime_error(std::string(message) + ": " + std::to_string(error));
    }
}

}

