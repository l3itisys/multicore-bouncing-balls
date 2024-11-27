cmake_minimum_required(VERSION 3.10)
project(bouncing_balls)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find required packages
find_package(OpenGL REQUIRED)
find_package(OpenCL REQUIRED)
find_package(GLEW REQUIRED)
find_package(glfw3 3.3 REQUIRED)

# Add executable
add_executable(bouncing_balls
    src/main.cpp
    src/GPUManager.cpp
    src/Renderer.cpp
    src/Simulation.cpp
)

# Include directories
target_include_directories(bouncing_balls
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${OpenCL_INCLUDE_DIRS}
        ${GLEW_INCLUDE_DIRS}
)

# Link libraries
target_link_libraries(bouncing_balls
    PRIVATE
        OpenGL::GL
        OpenCL::OpenCL
        GLEW::GLEW
        glfw
)

# Copy OpenCL kernel to build directory
add_custom_command(
    TARGET bouncing_balls POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy
            ${CMAKE_SOURCE_DIR}/src/kernels/simulation.cl
            ${CMAKE_CURRENT_BINARY_DIR}/simulation.cl
)
