cmake_minimum_required(VERSION 3.10)
project(BouncingBallsSimulation)

set(CMAKE_CXX_STANDARD 17)
set(OpenGL_GL_PREFERENCE GLVND)

find_package(OpenGL REQUIRED)
find_package(glfw3 3.3 REQUIRED)
find_package(GLEW REQUIRED)
find_package(TBB REQUIRED)

include_directories(
    ${OPENGL_INCLUDE_DIRS}
    ${GLEW_INCLUDE_DIRS}
    ${TBB_INCLUDE_DIRS}
    ${CMAKE_SOURCE_DIR}/include
)

set(SOURCE_FILES
    src/main.cpp
    src/Ball.cpp
    src/Simulation.cpp
    src/Renderer.cpp
)

add_executable(BouncingBallsSimulation ${SOURCE_FILES})

target_link_libraries(BouncingBallsSimulation
    ${OPENGL_LIBRARIES}
    glfw
    ${GLEW_LIBRARIES}
    TBB::tbb
)

