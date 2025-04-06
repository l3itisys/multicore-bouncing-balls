cmake_minimum_required(VERSION 3.10)
cmake_policy(SET CMP0072 NEW)
set(OpenGL_GL_PREFERENCE GLVND)

project(BouncingBallsSimulation)

# Set C++17 standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17")

# Find required packages
find_package(OpenGL REQUIRED)
find_package(glfw3 3.3 REQUIRED)
find_package(GLEW REQUIRED)
find_package(TBB REQUIRED)

# Include directories
include_directories(
    ${OPENGL_INCLUDE_DIRS}
    ${GLEW_INCLUDE_DIRS}
    ${TBB_INCLUDE_DIRS}
    ${CMAKE_SOURCE_DIR}/include
)

# Source files
set(SOURCE_FILES
    src/main.cpp
    src/Ball.cpp
    src/Simulation.cpp
    src/Renderer.cpp
    src/Grid.cpp
)

# Create executable
add_executable(BouncingBallsSimulation ${SOURCE_FILES})

# Link libraries
target_link_libraries(BouncingBallsSimulation
    ${OPENGL_LIBRARIES}
    glfw
    ${GLEW_LIBRARIES}
    TBB::tbb
)

