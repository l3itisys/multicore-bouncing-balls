#include "GPUManager.h"
#include "Config.h"
#include <fstream>
#include <iostream>
#include <filesystem>
#include <chrono>
#include <algorithm>
#include <sstream>
#include <X11/Xlib.h>
#include <GL/glx.h>
#include <GL/gl.h>

#ifdef _WIN32
#include <windows.h>
#else
#include <dlfcn.h>
#endif

namespace sim {

GPUManager::~GPUManager() {
    if (initialized) {
        cleanup();
    }
}

void GPUManager::cleanup() {
    try {
        // Cleanup OpenGL interop objects
        if (glInterop.initialized) {
            if (glInterop.textureId) {
                glDeleteTextures(1, &glInterop.textureId);
            }
            if (glInterop.vertexBuffer) {
                glDeleteBuffers(1, &glInterop.vertexBuffer);
            }
            glInterop.initialized = false;
        }
        // OpenCL resources are automatically cleaned up
        initialized = false;
        currentBufferSize = 0;
    } catch (const std::exception& e) {
        std::cerr << "Error during cleanup: " << e.what() << std::endl;
    }
}

void GPUManager::initialize(size_t numBalls, int screenWidth, int screenHeight) {
    if (initialized && numBalls <= currentBufferSize) {
        return;
    }

    try {
        // Store dimensions
        screenDimensions.width = screenWidth;
        screenDimensions.height = screenHeight;

        createContext();
        setupOpenGLInterop();
        buildProgram();
        createKernels();
        createBuffers(numBalls, screenWidth, screenHeight);

        initialized = true;
        currentBufferSize = numBalls;

        #ifdef _DEBUG
        printDeviceInfo();
        validateKernelWorkGroupSize();
        #endif
    } catch (const cl::Error& e) {
        handleOpenCLError(e, "Initialization");
        cleanup();
        throw;
    }
}

void GPUManager::createContext() {
    try {
        // Select platform and device
        platform = selectPlatform();
        device = selectDevice(platform);

        // Print platform info
        std::string platformName = platform.getInfo<CL_PLATFORM_NAME>();
        std::string vendorName = platform.getInfo<CL_PLATFORM_VENDOR>();
        std::cout << "Using platform: " << platformName << " from " << vendorName << std::endl;

        // Get current display and context
        Display* display = glXGetCurrentDisplay();
        GLXContext glxContext = glXGetCurrentContext();

        if (!display || !glxContext) {
            throw std::runtime_error("No active OpenGL context found");
        }

        // Create context with GL sharing
        cl_int error = CL_SUCCESS;
        cl_context_properties props[] = {
            CL_CONTEXT_PLATFORM, (cl_context_properties)platform(),
            CL_GLX_DISPLAY_KHR, (cl_context_properties)display,
            CL_GL_CONTEXT_KHR, (cl_context_properties)glxContext,
            0
        };

        std::vector<cl::Device> devices = {device};
        context = cl::Context(devices, props, nullptr, nullptr, &error);

        if (error != CL_SUCCESS) {
            std::stringstream ss;
            ss << "Failed to create OpenCL context with error: " << error;
            throw cl::Error(error, ss.str().c_str());
        }

        // Create command queues
        cl_command_queue_properties queueProps = CL_QUEUE_PROFILING_ENABLE;
        computeQueue = cl::CommandQueue(context, device, queueProps, &error);
        if (error != CL_SUCCESS) {
            throw cl::Error(error, "Failed to create compute queue");
        }

        transferQueue = cl::CommandQueue(context, device, queueProps, &error);
        if (error != CL_SUCCESS) {
            throw cl::Error(error, "Failed to create transfer queue");
        }

        std::cout << "Successfully created OpenCL context with OpenGL sharing" << std::endl;

        // Additional validation step
        auto contextDevices = context.getInfo<CL_CONTEXT_DEVICES>();
        if (contextDevices.empty()) {
            throw std::runtime_error("Created context has no devices");
        }

        // Get actual context properties for verification
        auto contextProps = context.getInfo<CL_CONTEXT_PROPERTIES>();
        std::cout << "Created context properties:" << std::endl;
        for (size_t i = 0; i < contextProps.size(); i += 2) {
            if (contextProps[i] == 0) break;
            std::cout << "Property " << contextProps[i] << ": " << contextProps[i + 1] << std::endl;
        }

    } catch (const cl::Error& e) {
        std::stringstream ss;
        ss << "Context creation error: " << e.what() << " (" << e.err() << ")\n";

        // Add Intel-specific error information
        if (e.err() == CL_INVALID_GL_SHAREGROUP_REFERENCE_KHR) {
            ss << "Invalid OpenGL share group. This might be due to:\n"
               << "1. OpenGL context not current\n"
               << "2. Incompatible OpenGL version\n"
               << "3. Intel driver specific issues\n";
        }

        std::cerr << ss.str();
        throw;
    }
}

void GPUManager::setupOpenGLInterop() {
    try {
        // Create and initialize OpenGL texture
        glGenTextures(1, &glInterop.textureId);
        glBindTexture(GL_TEXTURE_2D, glInterop.textureId);

        // Initialize texture storage
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8,
                    screenDimensions.width, screenDimensions.height,
                    0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);

        // Set texture parameters
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

        glBindTexture(GL_TEXTURE_2D, 0);

        // Create OpenCL-GL texture interop
        glFinish();  // Ensure GL is done
        cl_int error = CL_SUCCESS;
        glInterop.textureCL = cl::ImageGL(
            context,
            CL_MEM_WRITE_ONLY,
            GL_TEXTURE_2D,
            0,
            glInterop.textureId,
            &error
        );

        if (error != CL_SUCCESS) {
            throw cl::Error(error, "Failed to create OpenCL-GL interop");
        }

        if (error != CL_SUCCESS) {
            throw cl::Error(error, "Failed to create OpenCL-GL interop");
        }

        glInterop.initialized = true;
    } catch (const cl::Error& e) {
        handleOpenCLError(e, "OpenGL Interop Setup");
        throw;
    }
}

void GPUManager::buildProgram() {
    try {
        std::string source = loadKernelSource();
        cl::Program::Sources sources;
        sources.push_back({source.c_str(), source.length()});

        program = cl::Program(context, sources);

        std::string options = "-cl-std=CL2.0 "
                            "-cl-mad-enable "
                            "-cl-fast-relaxed-math "
                            "-cl-no-signed-zeros ";
        #ifdef _DEBUG
        options += "-g -cl-opt-disable ";
        #endif

        program.build(options.c_str());
    } catch (const cl::Error& e) {
        std::cerr << "Build error:\n"
                  << program.getBuildInfo<CL_PROGRAM_BUILD_LOG>(device)
                  << std::endl;
        throw;
    }
}

void GPUManager::createKernels() {
    try {
        physicsKernel = cl::Kernel(program, "updateBallPhysics");
        collisionKernel = cl::Kernel(program, "detectCollisions");
        renderKernel = cl::Kernel(program, "updatePixelBuffer");
    } catch (const cl::Error& e) {
        handleOpenCLError(e, "Kernel Creation");
        throw;
    }
}

std::string GPUManager::loadKernelSource() {
    std::filesystem::path kernelPath = std::filesystem::current_path() /
                                     config::OpenCL::KERNEL_FILENAME;
    if (!std::filesystem::exists(kernelPath)) {
        throw std::runtime_error(config::Error::KERNEL_FILE_NOT_FOUND);
    }

    std::ifstream file(kernelPath);
    if (!file.is_open()) {
        throw std::runtime_error("Failed to open kernel file: " + kernelPath.string());
    }

    return std::string(
        std::istreambuf_iterator<char>(file),
        std::istreambuf_iterator<char>()
    );
}

void GPUManager::createBuffers(size_t numBalls, int /*screenWidth*/, int /*screenHeight*/) {
    try {
        // Calculate buffer sizes
        size_t ballBufferSize = sizeof(Ball) * numBalls;
        size_t gridBufferSize = sizeof(cl_int) * config::OpenCL::GRID_SIZE *
                               config::OpenCL::GRID_SIZE * config::OpenCL::MAX_BALLS_PER_CELL;

        // Create ball buffer with host pointer access
        ballBuffer = cl::Buffer(
            context,
            config::MemoryFlags::BALL_BUFFER,
            ballBufferSize
        );

        // Create constants buffer
        constantsBuffer = cl::Buffer(
            context,
            config::MemoryFlags::CONSTANT_BUFFER,
            sizeof(SimConstants)
        );

        // Create grid buffer for spatial partitioning
        gridBuffer = cl::Buffer(
            context,
            CL_MEM_READ_WRITE,
            gridBufferSize
        );

        #ifdef _DEBUG
        validateBufferSizes();
        #endif
    } catch (const cl::Error& e) {
        handleOpenCLError(e, "Buffer Creation");
        throw;
    }
}

void GPUManager::updateSimulation(std::vector<Ball>& balls, const SimConstants& constants) {
    try {
        startProfiling();

        // Ensure OpenGL is done
        glFinish();

        // Acquire GL objects
        if (glInterop.initialized) {
            std::vector<cl::Memory> glObjects = {glInterop.textureCL};
            computeQueue.enqueueAcquireGLObjects(&glObjects);
        }

        // Write data to device (non-blocking)
        transferQueue.enqueueWriteBuffer(
            ballBuffer,
            CL_FALSE,  // Non-blocking write
            0,
            sizeof(Ball) * balls.size(),
            balls.data(),
            nullptr,
            &profiling.events.emplace_back()
        );

        transferQueue.enqueueWriteBuffer(
            constantsBuffer,
            CL_FALSE,
            0,
            sizeof(SimConstants),
            &constants,
            nullptr,
            &profiling.events.emplace_back()
        );

        // Physics update kernel
        physicsKernel.setArg(0, ballBuffer);
        physicsKernel.setArg(1, constantsBuffer);
        physicsKernel.setArg(2, static_cast<int>(balls.size()));

        computeQueue.enqueueNDRangeKernel(
            physicsKernel,
            cl::NullRange,
            cl::NDRange(balls.size()),
            cl::NDRange(std::min(balls.size(), WORKGROUP_SIZE)),
            nullptr,
            &profiling.events.emplace_back()
        );

        // Acquire GL objects before rendering
        std::vector<cl::Memory> glObjects = {glInterop.textureCL};
        computeQueue.enqueueAcquireGLObjects(
            &glObjects,
            nullptr,
            &profiling.events.emplace_back()
        );

        // Update pixel buffer
        renderKernel.setArg(0, ballBuffer);
        renderKernel.setArg(1, glInterop.textureCL);
        renderKernel.setArg(2, screenDimensions.width);
        renderKernel.setArg(3, screenDimensions.height);
        renderKernel.setArg(4, static_cast<int>(balls.size()));

        computeQueue.enqueueNDRangeKernel(
            renderKernel,
            cl::NullRange,
            cl::NDRange(screenDimensions.width, screenDimensions.height),
            cl::NDRange(16, 16),
            nullptr,
            &profiling.events.emplace_back()
        );

        // Release GL objects
        computeQueue.enqueueReleaseGLObjects(
            &glObjects,
            nullptr,
            &profiling.events.emplace_back()
        );

        // Collision detection with spatial partitioning
        collisionKernel.setArg(0, ballBuffer);
        collisionKernel.setArg(1, constantsBuffer);
        collisionKernel.setArg(2, cl::Local(sizeof(Ball) * WORKGROUP_SIZE));
        collisionKernel.setArg(3, static_cast<int>(balls.size()));

        computeQueue.enqueueNDRangeKernel(
            collisionKernel,
            cl::NullRange,
            cl::NDRange(balls.size()),
            cl::NDRange(WORKGROUP_SIZE),
            nullptr,
            &profiling.events.emplace_back()
        );

        // Read back results (blocking)
        transferQueue.enqueueReadBuffer(
            ballBuffer,
            CL_TRUE,
            0,
            sizeof(Ball) * balls.size(),
            balls.data(),
            nullptr,
            &profiling.events.emplace_back()
        );

        endProfiling("Complete simulation cycle");
    } catch (const cl::Error& e) {
        handleOpenCLError(e, "Simulation Update");
        throw;
    }
}

void GPUManager::updateDisplay() {
    if (!glInterop.initialized) return;

    try {
        // Bind the texture for display
        glBindTexture(GL_TEXTURE_2D, glInterop.textureId);
        glBegin(GL_QUADS);
        glTexCoord2f(0.0f, 0.0f); glVertex2f(-1.0f, -1.0f);
        glTexCoord2f(1.0f, 0.0f); glVertex2f( 1.0f, -1.0f);
        glTexCoord2f(1.0f, 1.0f); glVertex2f( 1.0f,  1.0f);
        glTexCoord2f(0.0f, 1.0f); glVertex2f(-1.0f,  1.0f);
        glEnd();
        glBindTexture(GL_TEXTURE_2D, 0);
    } catch (const std::exception& e) {
        std::cerr << "Display update error: " << e.what() << std::endl;
        throw;
    }
}

void GPUManager::synchronizeState(std::vector<Ball>& balls) {
    try {
        transferQueue.enqueueReadBuffer(
            ballBuffer,
            CL_TRUE,  // Blocking read
            0,
            sizeof(Ball) * balls.size(),
            balls.data()
        );
    } catch (const cl::Error& e) {
        handleOpenCLError(e, "State Synchronization");
        throw;
    }
}


cl::Platform GPUManager::selectPlatform() {
    std::vector<cl::Platform> platforms;
    cl::Platform::get(&platforms);

    if (platforms.empty()) {
        throw std::runtime_error("No OpenCL platforms found");
    }

    // First try to find a platform with GPU devices
    for (const auto& platform : platforms) {
        try {
            std::string name = platform.getInfo<CL_PLATFORM_NAME>();
            std::string version = platform.getInfo<CL_PLATFORM_VERSION>();
            std::string extensions = platform.getInfo<CL_PLATFORM_EXTENSIONS>();

            std::cout << "Found platform: " << name << "\n"
                      << "Version: " << version << "\n"
                      << "Extensions: " << extensions << std::endl;

            std::vector<cl::Device> devices;
            platform.getDevices(CL_DEVICE_TYPE_GPU, &devices);

            if (!devices.empty()) {
                std::cout << "Selected platform: " << name << std::endl;
                return platform;
            }
        } catch (...) {
            continue;
        }
    }

    throw std::runtime_error("No OpenCL platform with GPU found");
}

cl::Device GPUManager::selectDevice(cl::Platform& platform) {
    std::vector<cl::Device> devices;

    try {
        platform.getDevices(CL_DEVICE_TYPE_GPU, &devices);
    } catch (const cl::Error&) {
        throw std::runtime_error("No GPU devices found");
    }

    for (const auto& device : devices) {
        try {
            std::string name = device.getInfo<CL_DEVICE_NAME>();
            std::string extensions = device.getInfo<CL_DEVICE_EXTENSIONS>();

            std::cout << "Found device: " << name << "\n"
                      << "Extensions: " << extensions << std::endl;

            if (extensions.find("cl_khr_gl_sharing") != std::string::npos) {
                std::cout << "Selected device with GL sharing: " << name << std::endl;
                return device;
            }
        } catch (...) {
            continue;
        }
    }

    throw std::runtime_error("No device found with OpenGL sharing support");
}

void GPUManager::startProfiling() {
    profiling.startTime = std::chrono::high_resolution_clock::now();
    profiling.events.clear();
}

void GPUManager::endProfiling(const std::string& /*kernelName*/) {
    auto endTime = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration<double>(endTime - profiling.startTime).count();

    double kernelTime = 0.0;
    double transferTime = 0.0;

    for (const auto& event : profiling.events) {
        try {
            cl_ulong start = event.getProfilingInfo<CL_PROFILING_COMMAND_START>();
            cl_ulong end = event.getProfilingInfo<CL_PROFILING_COMMAND_END>();
            double eventTime = (end - start) * 1e-9; // Convert to seconds

            cl_command_type type = event.getInfo<CL_EVENT_COMMAND_TYPE>();
            if (type == CL_COMMAND_NDRANGE_KERNEL) {
                kernelTime += eventTime;
            } else {
                transferTime += eventTime;
            }
        } catch (const cl::Error& e) {
            std::cerr << "Profiling error: " << e.what() << std::endl;
        }
    }

    // Update performance statistics
    stats.computeTime = kernelTime * 1000.0;  // Convert to milliseconds
    stats.transferTime = transferTime * 1000.0;
    stats.renderTime = duration * 1000.0 - (kernelTime + transferTime) * 1000.0;
    stats.frameCount++;
    stats.gpuMemoryUsage = calculateGPUMemoryUsage();
}

void GPUManager::handleOpenCLError(const cl::Error& error, const char* location) {
    std::stringstream ss;
    ss << "OpenCL error in " << location << ": "
       << error.what() << " (" << error.err() << ")";

    #ifdef _DEBUG
    if (error.err() == CL_BUILD_PROGRAM_FAILURE) {
        ss << "\nBuild log:\n" << program.getBuildInfo<CL_PROGRAM_BUILD_LOG>(device);
    }
    #endif

    std::cerr << ss.str() << std::endl;
    throw error;
}

size_t GPUManager::calculateGPUMemoryUsage() const {
    size_t totalUsage = 0;

    // Ball buffer size
    totalUsage += currentBufferSize * sizeof(Ball);

    // Constants buffer size
    totalUsage += sizeof(SimConstants);

    // Grid buffer size (if used)
    totalUsage += config::OpenCL::GRID_SIZE * config::OpenCL::GRID_SIZE *
                  config::OpenCL::MAX_BALLS_PER_CELL * sizeof(cl_int);

    // Texture size
    totalUsage += screenDimensions.width * screenDimensions.height * 4; // RGBA

    return totalUsage;
}

void GPUManager::printDeviceInfo() {
    std::cout << "\nOpenCL Device Information:\n"
              << "  Device: " << device.getInfo<CL_DEVICE_NAME>() << "\n"
              << "  Vendor: " << device.getInfo<CL_DEVICE_VENDOR>() << "\n"
              << "  Version: " << device.getInfo<CL_DEVICE_VERSION>() << "\n"
              << "  Driver Version: " << device.getInfo<CL_DRIVER_VERSION>() << "\n"
              << "  Compute Units: " << device.getInfo<CL_DEVICE_MAX_COMPUTE_UNITS>() << "\n"
              << "  Max Work Group Size: " << device.getInfo<CL_DEVICE_MAX_WORK_GROUP_SIZE>() << "\n"
              << "  Global Memory: " << (device.getInfo<CL_DEVICE_GLOBAL_MEM_SIZE>() / (1024*1024)) << " MB\n"
              << "  Local Memory: " << (device.getInfo<CL_DEVICE_LOCAL_MEM_SIZE>() / 1024) << " KB\n"
              << "  Max Allocation Size: " << (device.getInfo<CL_DEVICE_MAX_MEM_ALLOC_SIZE>() / (1024*1024)) << " MB\n"
              << std::endl;
}

#ifdef _DEBUG
void GPUManager::validateBufferSizes() {
    // Check maximum allocation size
    cl_ulong maxAlloc = device.getInfo<CL_DEVICE_MAX_MEM_ALLOC_SIZE>();
    cl_ulong globalMem = device.getInfo<CL_DEVICE_GLOBAL_MEM_SIZE>();

    // Calculate required memory
    size_t totalRequired = calculateGPUMemoryUsage();
    if (totalRequired > globalMem) {
        throw std::runtime_error("Total required memory exceeds device global memory");
    }

    // Check individual buffer sizes
    size_t ballBufferSize = currentBufferSize * sizeof(Ball);
    if (ballBufferSize > maxAlloc) {
        throw std::runtime_error("Ball buffer size exceeds maximum allocation size");
    }

    size_t textureSize = screenDimensions.width * screenDimensions.height * 4;
    if (textureSize > maxAlloc) {
        throw std::runtime_error("Texture size exceeds maximum allocation size");
    }
}

void GPUManager::validateKernelWorkGroupSize() {
    try {
        // Get maximum work group size for each kernel
        size_t maxWorkGroupSize = device.getInfo<CL_DEVICE_MAX_WORK_GROUP_SIZE>();

        // Check physics kernel
        size_t physicsWGS = physicsKernel.getWorkGroupInfo<CL_KERNEL_WORK_GROUP_SIZE>(device);
        if (WORKGROUP_SIZE > physicsWGS || WORKGROUP_SIZE > maxWorkGroupSize) {
            std::cerr << "Warning: Physics kernel work group size may be too large\n"
                      << "Maximum supported: " << physicsWGS
                      << ", Current: " << WORKGROUP_SIZE << std::endl;
        }

        // Check collision kernel
        size_t collisionWGS = collisionKernel.getWorkGroupInfo<CL_KERNEL_WORK_GROUP_SIZE>(device);
        if (WORKGROUP_SIZE > collisionWGS || WORKGROUP_SIZE > maxWorkGroupSize) {
            std::cerr << "Warning: Collision kernel work group size may be too large\n"
                      << "Maximum supported: " << collisionWGS
                      << ", Current: " << WORKGROUP_SIZE << std::endl;
        }

        // Check render kernel
        size_t renderWGS = renderKernel.getWorkGroupInfo<CL_KERNEL_WORK_GROUP_SIZE>(device);
        if (16 * 16 > renderWGS || 16 * 16 > maxWorkGroupSize) {
            std::cerr << "Warning: Render kernel work group size may be too large\n"
                      << "Maximum supported: " << renderWGS
                      << ", Current: " << (16 * 16) << std::endl;
        }
    } catch (const cl::Error& e) {
        std::cerr << "Warning: Failed to validate kernel work group sizes: "
                  << e.what() << std::endl;
    }
}
#endif // _DEBUG

} // namespace sim
