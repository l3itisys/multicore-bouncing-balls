cmake_minimum_required(VERSION 3.10)
project(BouncingBallsSimulation)

find_package(TBB REQUIRED)
find_package(OpenGL REQUIRED)
find_package(GLM REQUIRED)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

add_executable(BouncingBallsSimulation
    main.cpp
    Simulation.cpp
    Ball.cpp
    Renderer.cpp
)

target_include_directories(BouncingBallsSimulation PRIVATE
    ${TBB_INCLUDE_DIRS}
    ${OPENGL_INCLUDE_DIR}
)

target_link_libraries(BouncingBallsSimulation PRIVATE
    ${TBB_IMPORTED_TARGETS}
    ${OPENGL_gl_LIBRARY}
    glfw
    GLEW::GLEW
)

