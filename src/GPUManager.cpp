#include "GPUManager.h"
#include <fstream>
#include <iostream>
#include <cmath>
#include <filesystem>
#include <sstream>
#include <GL/glew.h>
#include <GLFW/glfw3.h>

#ifdef __linux__
#include <GL/glx.h>
#endif

namespace sim {

GPUManager::~GPUManager() {
    if (glTextureID != 0) {
        glDeleteTextures(1, &glTextureID);
    }
}

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

        // Store screen dimensions
        screenDimensions.width = screenWidth;
        screenDimensions.height = screenHeight;

        // Print OpenCL device information
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

    // First try to find Intel platform
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

    // Check for OpenGL-OpenCL interop support
    std::string extensions = device.getInfo<CL_DEVICE_EXTENSIONS>();
    if (extensions.find("cl_khr_gl_sharing") == std::string::npos) {
        std::cerr << "Device does not support OpenGL-OpenCL interop (cl_khr_gl_sharing)" << std::endl;
        throw std::runtime_error("OpenGL-OpenCL interop not supported");
    }

    std::cout << "Using OpenCL device: "
              << device.getInfo<CL_DEVICE_NAME>()
              << " from "
              << platform.getInfo<CL_PLATFORM_VENDOR>()
              << "\nDevice extensions: " << extensions << std::endl;

    // Get current OpenGL context and display
    GLFWwindow* currentWindow = glfwGetCurrentContext();
    if (!currentWindow) {
        throw std::runtime_error("No current OpenGL context");
    }

    // Get current OpenGL context and display
#ifdef __linux__
    Display* display = glXGetCurrentDisplay();
    GLXContext glxContext = glXGetCurrentContext();
    
    if (!display || !glxContext) {
        throw std::runtime_error("Failed to get GLX context or display");
    }
    
    // Set up context properties
    std::vector<cl_context_properties> properties = {
        CL_CONTEXT_PLATFORM, (cl_context_properties)(platform)(),
        CL_GL_CONTEXT_KHR, (cl_context_properties)glxContext,
        CL_GLX_DISPLAY_KHR, (cl_context_properties)display,
        0
    };
#else
    #error "Platform not supported"
#endif

    // Debug output
    std::cout << "Creating OpenCL context with properties:" << std::endl;
    for (size_t i = 0; i < properties.size() - 1; i += 2) {
        std::cout << "Property " << properties[i] << ": " << properties[i + 1] << std::endl;
    }

    // Create context
    cl_int err = CL_SUCCESS;
    try {
        // Make sure OpenGL is done with any commands
        glFinish();
        
        // Create the OpenCL context
        context = cl::Context(
            {device},
            properties.data(),
            nullptr,
            nullptr,
            &err
        );

        if (err != CL_SUCCESS) {
            std::stringstream ss;
            ss << "OpenCL context creation failed (error " << err << "): ";
            switch (err) {
                case CL_INVALID_PLATFORM: ss << "CL_INVALID_PLATFORM"; break;
                case CL_INVALID_PROPERTY: ss << "CL_INVALID_PROPERTY"; break;
                case CL_INVALID_VALUE: ss << "CL_INVALID_VALUE"; break;
                case CL_INVALID_DEVICE: ss << "CL_INVALID_DEVICE"; break;
                case CL_DEVICE_NOT_AVAILABLE: ss << "CL_DEVICE_NOT_AVAILABLE"; break;
                case CL_OUT_OF_RESOURCES: ss << "CL_OUT_OF_RESOURCES"; break;
                case CL_OUT_OF_HOST_MEMORY: ss << "CL_OUT_OF_HOST_MEMORY"; break;
                default: ss << "Unknown error";
            }
            throw std::runtime_error(ss.str());
        }
    } catch (const cl::Error& e) {
        std::cerr << "OpenCL error: " << e.what() << " (" << e.err() << ")" << std::endl;
        throw;
    }
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

void GPUManager::createGLTexture(int width, int height) {
    // Create OpenGL texture
    glGenTextures(1, &glTextureID);
    glBindTexture(GL_TEXTURE_2D, glTextureID);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA,
                 width, height, 0, GL_RGBA,
                 GL_UNSIGNED_BYTE, nullptr);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glBindTexture(GL_TEXTURE_2D, 0);

    // Create OpenCL image from OpenGL texture
    cl::ImageGL image(
        context,
        CL_MEM_WRITE_ONLY,
        GL_TEXTURE_2D,
        0,
        glTextureID
    );

    pixelImage = image;
}

void GPUManager::createBuffers(size_t numBalls, int screenWidth, int screenHeight) {
    // Calculate buffer sizes
    size_t ballBufferSize = sizeof(Ball) * numBalls;

    // Create buffers with proper memory flags
    ballBuffer = cl::Buffer(
        context,
        CL_MEM_READ_WRITE | CL_MEM_ALLOC_HOST_PTR,
        ballBufferSize
    );

    constantsBuffer = cl::Buffer(
        context,
        CL_MEM_READ_ONLY | CL_MEM_ALLOC_HOST_PTR,
        sizeof(SimConstants)
    );

    // Create OpenGL texture and corresponding OpenCL image
    createGLTexture(screenWidth, screenHeight);
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

        // Detect and resolve collisions
        collisionKernel.setArg(0, ballBuffer);
        collisionKernel.setArg(1, constantsBuffer);
        collisionKernel.setArg(2, static_cast<int>(balls.size()));

        size_t numCollisionThreads = balls.size();
        queue.enqueueNDRangeKernel(
            collisionKernel,
            cl::NullRange,
            cl::NDRange(numCollisionThreads),
            cl::NDRange(std::min(numCollisionThreads, WORKGROUP_SIZE))
        );

        // Synchronize queue
        queue.finish();

    } catch (const cl::Error& e) {
        std::cerr << "OpenCL error in updateSimulation: "
                  << e.what() << " (" << e.err() << ")" << std::endl;
        throw;
    }
}

void GPUManager::updateDisplay() {
    try {
        // Acquire OpenGL texture for OpenCL
        glFinish(); // Ensure OpenGL is done with the texture
        std::vector<cl::Memory> glObjects = { pixelImage };
        queue.enqueueAcquireGLObjects(&glObjects);

        // Update pixels - many threads for parallel rendering
        pixelUpdateKernel.setArg(0, ballBuffer);
        pixelUpdateKernel.setArg(1, pixelImage);
        pixelUpdateKernel.setArg(2, constantsBuffer);
        pixelUpdateKernel.setArg(3, screenDimensions.width);
        pixelUpdateKernel.setArg(4, screenDimensions.height);
        pixelUpdateKernel.setArg(5, static_cast<int>(currentBufferSize));

        // Use many threads for pixel updates
        size_t globalWorkSizeX = (screenDimensions.width + 15) / 16;
        size_t globalWorkSizeY = (screenDimensions.height + 15) / 16;

        cl::NDRange globalSize(globalWorkSizeX * 16, globalWorkSizeY * 16);
        cl::NDRange localSize(16, 16);

        queue.enqueueNDRangeKernel(
            pixelUpdateKernel,
            cl::NullRange,
            globalSize,
            localSize
        );

        // Release OpenGL texture from OpenCL
        queue.enqueueReleaseGLObjects(&glObjects);
        queue.finish();

    } catch (const cl::Error& e) {
        std::cerr << "OpenCL error in updateDisplay: "
                  << e.what() << " (" << e.err() << ")" << std::endl;
        throw;
    }
}

void GPUManager::synchronizeState(std::vector<Ball>& balls) {
    // No need to read back the balls if not necessary
}

void GPUManager::checkError(cl_int error, const char* message) {
    if (error != CL_SUCCESS) {
        throw std::runtime_error(std::string(message) + ": " + std::to_string(error));
    }
}

} // namespace sim

