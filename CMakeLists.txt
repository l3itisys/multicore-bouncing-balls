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
# Remove or comment out the GLUT find_package
# find_package(GLUT REQUIRED)

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
        # Remove or comment out GLUT include dirs
        # ${GLUT_INCLUDE_DIRS}
)

# Link libraries
target_link_libraries(bouncing_balls
    PRIVATE
        OpenGL::GL
        OpenCL::OpenCL
        GLEW::GLEW
        glfw
        # Remove or comment out GLUT libraries
        # ${GLUT_LIBRARIES}
)

# Copy OpenCL kernel to build directory
add_custom_command(
    TARGET bouncing_balls POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy
            ${CMAKE_SOURCE_DIR}/src/kernels/simulation.cl
            ${CMAKE_CURRENT_BINARY_DIR}/simulation.cl
)

# Handle CMake policies to suppress warnings
if(POLICY CMP0072)
    cmake_policy(SET CMP0072 NEW)
endif()

if(POLICY CMP0074)
    cmake_policy(SET CMP0074 NEW)
endif()

