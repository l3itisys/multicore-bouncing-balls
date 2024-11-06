#ifndef BOUNCING_BALLS_RENDERER_H
#define BOUNCING_BALLS_RENDERER_H

#include "Types.h"
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <vector>
#include <memory>
#include <string>

namespace sim {

class Renderer {
public:
    Renderer(int width, int height);
    ~Renderer();

    Renderer(const Renderer&) = delete;
    Renderer& operator=(const Renderer&) = delete;

    bool initialize();
    void render(const std::vector<Ball>& balls, double fps);
    bool shouldClose() const;

    int getWidth() const { return width; }
    int getHeight() const { return height; }

private:
    class GLFWContext {
    public:
        GLFWContext();
        ~GLFWContext();
    };

    void setupOpenGL();
    void renderFPS(double fps);
    void drawBall(const Ball& ball);
    static void framebufferSizeCallback(GLFWwindow* window, int width, int height);

    void drawText(const std::string& text, float x, float y, float scale = 1.0f);
    void drawCircle(float x, float y, float radius, uint32_t color, float alpha);

    int width;
    int height;
    GLFWwindow* window;
    static GLFWContext glfw;  // Static GLFW initialization

    // Constants
    static constexpr int CIRCLE_SEGMENTS = 32;
    static constexpr float PI = 3.14159265358979323846f;
    static constexpr float TEXT_SCALE = 0.15f;
};

}

#endif // BOUNCING_BALLS_RENDERER_H
\documentclass[12pt]{article}
\usepackage[utf8]{inputenc}
\usepackage{graphicx}
\usepackage{listings}
\usepackage{color}
\usepackage{geometry}
\usepackage{hyperref}

\geometry{margin=1in}

\title{Parallel Bouncing Balls Simulation Using OpenCL and OpenGL}
\author{[Your Name]}
\date{\today}

\begin{document}
\maketitle

\section{Introduction}
This report discusses the implementation of a parallel bouncing balls simulation using OpenCL for physics computations and OpenGL for rendering. The project demonstrates the effective use of GPU acceleration for both physics calculations and graphics rendering, while maintaining smooth performance through multi-threaded execution.

\section{Technical Implementation}

\subsection{Architecture Overview}
The simulation is built on a multi-threaded architecture with two main threads:
\begin{itemize}
    \item Control Thread: Manages display updates at 30 FPS
    \item Computation Thread: Handles GPU-accelerated physics calculations
\end{itemize}

The separation of rendering and computation allows for optimal resource utilization and smooth visual output, even under heavy computational loads.

\subsection{Key Components}
\subsubsection{GPU Manager}
The GPUManager class serves as the interface to OpenCL, handling:
\begin{itemize}
    \item Kernel compilation and buffer management
    \item Physics computations including collision detection
    \item State synchronization between CPU and GPU
\end{itemize}

\subsubsection{Renderer}
The OpenGL-based renderer implements:
\begin{itemize}
    \item Efficient ball rendering with proper Z-ordering
    \item Anti-aliasing and alpha blending for smooth visuals
    \item Real-time FPS display and window management
\end{itemize}

\section{Performance Considerations}
Several optimizations were implemented to ensure optimal performance:

\begin{itemize}
    \item Work group size optimization for Intel GPUs (256 threads)
    \item Efficient memory management with aligned structures
    \item Parallel collision detection using multiple GPU threads
    \item Frame timing control to maintain consistent 30 FPS
\end{itemize}

\section{Challenges and Solutions}
The main challenges encountered during development included:

\begin{itemize}
    \item Synchronization between CPU and GPU memory
    \item Collision detection optimization for large numbers of balls
    \item Maintaining consistent frame rates under varying loads
\end{itemize}

These were addressed through careful thread synchronization, efficient GPU kernel design, and proper work distribution among available compute units.

\section{Conclusion}
The project successfully demonstrates the power of parallel computing in physics simulations. The combination of OpenCL and OpenGL, along with proper thread management, results in a smooth and efficient simulation capable of handling multiple balls while maintaining consistent performance.

Future improvements could include:
\begin{itemize}
    \item Advanced collision response algorithms
    \item Dynamic work group size adjustment
    \item Additional visual effects using compute shaders
\end{itemize}

\end{document}
