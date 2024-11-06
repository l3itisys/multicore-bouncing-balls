cmake_minimum_required(VERSION 3.10)
project(BouncingBalls)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find required packages
find_package(OpenGL REQUIRED)
find_package(OpenCL REQUIRED)
find_package(GLEW REQUIRED)
find_package(glfw3 3.3 REQUIRED)

# Add all source files
set(SOURCES
    src/main.cpp
    src/GPUManager.cpp
    src/Renderer.cpp
    src/Simulation.cpp
)

# Create executable
add_executable(bouncing_balls ${SOURCES})

# Include directories
target_include_directories(bouncing_balls PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# Link libraries
target_link_libraries(bouncing_balls PRIVATE
    OpenGL::GL
    OpenCL::OpenCL
    GLEW::GLEW
    glfw
)

# On Linux, link against X11 libraries
if(UNIX AND NOT APPLE)
    find_package(X11 REQUIRED)
    if(X11_FOUND)
        target_include_directories(bouncing_balls PRIVATE ${X11_INCLUDE_DIR})
        target_link_libraries(bouncing_balls PRIVATE ${X11_LIBRARIES})
    endif()
endif()

# Copy OpenCL kernel to build directory
configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/src/kernels/simulation.cl
    ${CMAKE_CURRENT_BINARY_DIR}/simulation.cl
    COPYONLY
)

cmake_minimum_required(VERSION 3.10)
project(BouncingBalls)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find required packages
find_package(OpenGL REQUIRED)
find_package(OpenCL REQUIRED)
find_package(GLEW REQUIRED)
find_package(glfw3 3.3 REQUIRED)

# Add all source files
set(SOURCES
    src/main.cpp
    src/GPUManager.cpp
    src/Renderer.cpp
    src/Simulation.cpp
)

# Create executable
add_executable(bouncing_balls ${SOURCES})

# Include directories
target_include_directories(bouncing_balls PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# Link libraries
target_link_libraries(bouncing_balls PRIVATE
    OpenGL::GL
    OpenCL::OpenCL
    GLEW::GLEW
    glfw
)

# On Linux, link against X11 libraries
if(UNIX AND NOT APPLE)
    find_package(X11 REQUIRED)
    if(X11_FOUND)
        target_include_directories(bouncing_balls PRIVATE ${X11_INCLUDE_DIR})
        target_link_libraries(bouncing_balls PRIVATE ${X11_LIBRARIES})
    endif()
endif()

# Copy OpenCL kernel to build directory
configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/src/kernels/simulation.cl
    ${CMAKE_CURRENT_BINARY_DIR}/simulation.cl
    COPYONLY
)
