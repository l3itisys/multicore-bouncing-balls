#include "GPUManager.h"
#include <fstream>
#include <iostream>
#include <filesystem>

namespace sim {

GPUManager::~GPUManager() {
    if (initialized) {
        cleanup();
    }
}

void GPUManager::cleanup() {
    initialized = false;
}

void GPUManager::initialize(size_t numBalls_, int screenWidth, int screenHeight) {
    try {
        if (initialized) return;

        numBalls = numBalls_;
        screen.width = screenWidth;
        screen.height = screenHeight;

        std::cout << "Initializing GPU manager with " << numBalls << " balls" << std::endl;

        createContext();
        buildProgram();
        createKernels();
        createBuffers();

        workGroupSize = std::min(
            device.getInfo<CL_DEVICE_MAX_WORK_GROUP_SIZE>(),
            size_t(256)
        );

        initialized = true;

    } catch (const std::exception& error) {
        std::cerr << "Error during initialization: " << error.what() << std::endl;
        cleanup();
        throw;
    }
}

void GPUManager::createContext() {
    try {
        // Get OpenCL platforms
        std::vector<cl::Platform> platforms;
        cl::Platform::get(&platforms);
        if (platforms.empty()) {
            throw std::runtime_error("No OpenCL platforms found");
        }

        // Choose the first platform and device
        for (const auto& platform : platforms) {
            std::vector<cl::Device> devices;
            platform.getDevices(CL_DEVICE_TYPE_GPU, &devices);

            if (!devices.empty()) {
                device = devices.front();
                context = cl::Context(device);
                break;
            }
        }

        if (!context()) {
            throw std::runtime_error("Failed to create OpenCL context");
        }

        queue = cl::CommandQueue(context, device);

        std::cout << "OpenCL context created successfully" << std::endl;

    } catch (const cl::Error& e) {
        std::cerr << "OpenCL error in createContext: " << e.what() << " (" << e.err() << ")" << std::endl;
        throw;
    }
}

void GPUManager::buildProgram() {
    try {
        std::string source = loadKernelSource();
        cl::Program::Sources sources;
        sources.push_back({source.c_str(), source.length()});
        program = cl::Program(context, sources);
        program.build("-cl-std=CL1.2");
    }
    catch (const cl::Error& error) {
        std::cerr << "Build error:" << std::endl;
        std::cerr << program.getBuildInfo<CL_PROGRAM_BUILD_LOG>(device) << std::endl;
        throw;
    }
}

std::string GPUManager::loadKernelSource() {
    std::filesystem::path kernelPath = std::filesystem::current_path() /
                                     config::OpenCL::KERNEL_FILENAME;

    std::ifstream file(kernelPath);
    if (!file.is_open()) {
        throw std::runtime_error("Failed to open kernel file: " + kernelPath.string());
    }

    return std::string(
        std::istreambuf_iterator<char>(file),
        std::istreambuf_iterator<char>()
    );
}

void GPUManager::createKernels() {
    physicsKernel = cl::Kernel(program, "updateBallPhysics");
    collisionKernel = cl::Kernel(program, "detectCollisions");
}

void GPUManager::createBuffers() {
    ballsBuffer = cl::Buffer(
        context,
        CL_MEM_READ_WRITE,
        sizeof(Ball) * numBalls
    );

    constantsBuffer = cl::Buffer(
        context,
        CL_MEM_READ_ONLY,
        sizeof(SimConstants)
    );
}

void GPUManager::updatePhysics(std::vector<Ball>& balls) {
    try {
        // Write balls data to device
        queue.enqueueWriteBuffer(ballsBuffer, CL_FALSE, 0,
                                 sizeof(Ball) * numBalls, balls.data());

        // Write constants to device
        queue.enqueueWriteBuffer(constantsBuffer, CL_FALSE, 0,
                                 sizeof(SimConstants), &constants);

        // Set kernel arguments for physics update
        physicsKernel.setArg(0, ballsBuffer);
        physicsKernel.setArg(1, constantsBuffer);
        physicsKernel.setArg(2, static_cast<int>(numBalls));

        // Calculate work sizes
        size_t globalSize = ((numBalls + workGroupSize - 1) / workGroupSize) * workGroupSize;

        // Run physics kernel
        queue.enqueueNDRangeKernel(
            physicsKernel,
            cl::NullRange,
            cl::NDRange(globalSize),
            cl::NDRange(workGroupSize)
        );

        // Set kernel arguments for collision detection
        collisionKernel.setArg(0, ballsBuffer);
        collisionKernel.setArg(1, constantsBuffer);
        collisionKernel.setArg(2, static_cast<int>(numBalls));

        // Run collision kernel
        queue.enqueueNDRangeKernel(
            collisionKernel,
            cl::NullRange,
            cl::NDRange(globalSize),
            cl::NDRange(workGroupSize)
        );

        // Read updated balls data back to host
        queue.enqueueReadBuffer(ballsBuffer, CL_TRUE, 0,
                                sizeof(Ball) * numBalls, balls.data());

        queue.finish();

    } catch (const cl::Error& error) {
        std::cerr << "OpenCL error in physics update: " << error.what()
                  << " (" << error.err() << ")" << std::endl;
        throw;
    }
}

} // namespace sim

