#ifndef BALL_H
#define BALL_H

#include <mutex>

class Ball {
public:
    Ball(int id, float radius, float mass, float x, float y, float vx, float vy, int color);

    void updatePosition(float dt);
    void applyGravity(float dt);
    void handleCollision(Ball& other);
    void checkBoundaryCollision(float screenWidth, float screenHeight);

    float getX() const;
    float getY() const;
    float getVx() const;
    float getVy() const;
    float getRadius() const;
    float getMass() const;
    int getColor() const;
    int getId() const;
    std::mutex& getMutex();

private:
    int id_;
    float radius_;
    float mass_;
    float x_, y_;
    float vx_, vy_;
    int color_;
    std::mutex mutex_;

    static constexpr float RESTITUTION = 0.8f; // Coefficient of restitution
};

#endif // BALL_H

