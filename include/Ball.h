#ifndef BALL_H
#define BALL_H

#include <mutex>

class Grid;

class Ball {
public:
    Ball(int id, float radius, float mass, float x, float y, float vx, float vy,
         int color, Grid& grid, float screenWidth, float screenHeight);
    ~Ball() = default;
    Ball(const Ball&) = delete;
    Ball& operator=(const Ball&) = delete;

    void updatePosition(float dt);
    void applyGravity(float dt);
    void checkBoundaryCollision();
    void detectCollisions();
    void handleCollision(Ball& other);

    void getPosition(float& x, float& y) const;
    float getVx() const;
    float getVy() const;
    float getRadius() const { return radius_; }
    float getMass() const { return mass_; }
    int getColor() const { return color_; }
    int getId() const { return id_; }

private:
    const int id_;
    const float radius_;
    const float mass_;
    float x_, y_;
    float vx_, vy_;
    const int color_;
    mutable std::mutex mtx_;
    Grid& grid_;
    const float screenWidth_;
    const float screenHeight_;

    static constexpr float RESTITUTION = 0.8f;
};

#endif // BALL_H

