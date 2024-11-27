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
        createBuffers(numBalls);

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
        std::vector<cl::Platform> platforms;
        cl::Platform::get(&platforms);
        if (platforms.empty()) {
            throw std::runtime_error("No OpenCL platforms found");
        }

        // Find suitable platform and device
        for (const auto& platform : platforms) {
            std::string platformName = platform.getInfo<CL_PLATFORM_NAME>();
            std::cout << "Found platform: " << platformName << std::endl;

            try {
                std::vector<cl::Device> devices;
                platform.getDevices(CL_DEVICE_TYPE_ALL, &devices);

                for (const auto& dev : devices) {
                    device = dev;
                    std::cout << "Selected device: " << dev.getInfo<CL_DEVICE_NAME>() << std::endl;
                    break;
                }
            }
            catch (const cl::Error& e) {
                continue;
            }

            if (device()) break;
        }

        if (!device()) {
            throw std::runtime_error("No suitable OpenCL device found");
        }

        context = cl::Context(device);
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
        program = cl::Program(context, source);
        program.build("-cl-std=CL2.0");
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

void GPUManager::createBuffers(size_t numBalls) {
    size_t ballBufferSize = sizeof(Ball) * numBalls;

    ballBuffer = cl::Buffer(
        context,
        CL_MEM_READ_WRITE,
        ballBufferSize
    );

    constantsBuffer = cl::Buffer(
        context,
        CL_MEM_READ_ONLY,
        sizeof(SimConstants)
    );
}

void GPUManager::updatePhysics(std::vector<Ball>& balls) {
    try {
        // Write data to device
        queue.enqueueWriteBuffer(ballBuffer, CL_FALSE, 0,
                               sizeof(Ball) * balls.size(), balls.data());

        queue.enqueueWriteBuffer(constantsBuffer, CL_FALSE, 0,
                               sizeof(SimConstants), &constants);

        // Set kernel arguments for physics update
        physicsKernel.setArg(0, ballBuffer);
        physicsKernel.setArg(1, constantsBuffer);
        physicsKernel.setArg(2, static_cast<int>(balls.size()));

        // Calculate work sizes
        size_t globalSize = ((balls.size() + workGroupSize - 1) / workGroupSize) * workGroupSize;

        // Run physics kernel
        queue.enqueueNDRangeKernel(
            physicsKernel,
            cl::NullRange,
            cl::NDRange(globalSize),
            cl::NDRange(workGroupSize)
        );

        // Set kernel arguments for collision detection
        collisionKernel.setArg(0, ballBuffer);
        collisionKernel.setArg(1, constantsBuffer);
        collisionKernel.setArg(2, cl::Local(sizeof(Ball) * workGroupSize));
        collisionKernel.setArg(3, static_cast<int>(balls.size()));

        // Run collision kernel
        queue.enqueueNDRangeKernel(
            collisionKernel,
            cl::NullRange,
            cl::NDRange(globalSize),
            cl::NDRange(workGroupSize)
        );

        // Read back results
        queue.enqueueReadBuffer(ballBuffer, CL_TRUE, 0,
                              sizeof(Ball) * balls.size(), balls.data());

        queue.finish();

    } catch (const cl::Error& error) {
        std::cerr << "OpenCL error in physics update: " << error.what()
                  << " (" << error.err() << ")" << std::endl;
        throw;
    }
}

} // namespace sim
