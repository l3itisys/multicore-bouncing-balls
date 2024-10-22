#ifndef BALL_H
#define BALL_H

#include <tbb/atomic.h>
#include <vector>
#include <tbb/tbb.h>

class Ball {
public:
    Ball(int id, float radius, float mass, float x, float y, float vx, float vy, int color);

    void start();
    void stop();

    // Accessors
    float getX() const;
    float getY() const;
    float getRadius() const;
    int getColor() const;
    int getId() const;
    float getVx() const;
    float getVy() const;
    float getMass() const;

    // Rendering data
    void getRenderingData(float& x, float& y, float& radius, int& color) const;

    // Collision handling
    void checkCollisionWithBoundary(float screenWidth, float screenHeight);
    void checkCollisionWithBalls(const std::vector<Ball*>& balls);

    // Update position and velocity
    void updatePosition(float dt);

    // Synchronization
    void waitForUpdate();
    void signalUpdate();

    // For collision detection
    const std::vector<Ball*>* allBalls_;

private:
    void run();

    int id_;
    float radius_;
    float mass_;
    tbb::atomic<float> x_, y_;
    tbb::atomic<float> vx_, vy_;
    int color_;

    std::atomic<bool> running_;
    tbb::task_group_context context_;

    // Synchronization primitives
    std::atomic<bool> updateReady_;
    std::atomic<bool> proceed_;

    // For threading
    tbb::task_arena arena_;
};

#endif // BALL_H

