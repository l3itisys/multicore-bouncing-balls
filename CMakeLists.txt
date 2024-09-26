cmake_minimum_required(VERSION 3.10)
project(MulticoreBouncingBallsSim)

set(CMAKE_CXX_STANDARD 11)

# Add source files
file(GLOB SRC_FILES src/*.cpp)

# Include directories
include_directories(${PROJECT_SOURCE_DIR}/include)
include_directories(/usr/include/glm)

# Find packages
find_package(OpenGL REQUIRED)
find_package(glfw3 3.3 REQUIRED)

# Executable
add_executable(BouncingBallsSim ${SRC_FILES})

# Link libraries
target_link_libraries(BouncingBallsSim PRIVATE OpenGL::GL glfw)
