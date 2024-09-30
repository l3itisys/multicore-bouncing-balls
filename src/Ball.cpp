// Ball.cpp
#include "Ball.h"
#include <cmath>
#include <iostream>
#include <algorithm>

Ball::Ball(int id, float radius, float mass, float x, float y, float vx, float vy, int color)
    : id_(id), radius_(radius), mass_(mass), x_(x), y_(y), vx_(vx), vy_(vy), color_(color) {}

void Ball::updatePosition(float dt) {
    x_ += vx_ * dt;
    y_ += vy_ * dt;
}

void Ball::applyGravity(float dt) {
    const float gravity = 10.81f; // Positive gravity pulls balls down
    const float airResistance = 0.1f; // Air resistance coefficient
    vy_ += gravity * dt;
    vx_ *= (1.0f - airResistance * dt);
    vy_ *= (1.0f - airResistance * dt);
}

void Ball::handleCollision(Ball& other) {
    float dx = other.x_ - x_;
    float dy = other.y_ - y_;
    float distanceSquared = dx * dx + dy * dy;
    float minDist = radius_ + other.radius_;
    float minDistSquared = minDist * minDist;

    if (distanceSquared < minDistSquared) {
        float distance = std::sqrt(distanceSquared);
        if (distance == 0.0f) {
            distance = 0.0001f; // Small epsilon to prevent division by zero
        }

        // Normal vector
        float nx = dx / distance;
        float ny = dy / distance;

        // Relative velocity
        float rvx = vx_ - other.vx_;
        float rvy = vy_ - other.vy_;

        // Velocity along the normal
        float velAlongNormal = rvx * nx + rvy * ny;

        // Do not resolve if velocities are separating
        if (velAlongNormal > 0)
            return;

        // Coefficient of restitution
        float e = RESTITUTION;

        // Calculate impulse scalar
        float j = -(1 + e) * velAlongNormal;
        j /= (1 / mass_ + 1 / other.mass_);

        // Apply impulse
        float impulsex = j * nx;
        float impulsey = j * ny;

        vx_ -= impulsex / mass_;
        vy_ -= impulsey / mass_;
        other.vx_ += impulsex / other.mass_;
        other.vy_ += impulsey / other.mass_;

        // Position correction to prevent sinking
        const float percent = 0.2f; // Penetration percentage to correct
        const float slop = 0.01f;   // Penetration allowance
        float penetration = minDist - distance;
        float correctionMagnitude = std::max(penetration - slop, 0.0f) / (1 / mass_ + 1 / other.mass_) * percent;

        float correctionX = correctionMagnitude * nx;
        float correctionY = correctionMagnitude * ny;

        x_ += correctionX / mass_;
        y_ += correctionY / mass_;
        other.x_ -= correctionX / other.mass_;
        other.y_ -= correctionY / other.mass_;
    }
}

void Ball::checkBoundaryCollision(float screenWidth, float screenHeight) {
    if (x_ - radius_ < 0) {
        x_ = radius_;
        vx_ = std::abs(vx_) * RESTITUTION;
    } else if (x_ + radius_ > screenWidth) {
        x_ = screenWidth - radius_;
        vx_ = -std::abs(vx_) * RESTITUTION;
    }

    if (y_ - radius_ < 0) {
        y_ = radius_;
        vy_ = std::abs(vy_) * RESTITUTION;
    } else if (y_ + radius_ > screenHeight) {
        y_ = screenHeight - radius_;
        vy_ = -std::abs(vy_) * RESTITUTION;
    }
}

float Ball::getX() const { return x_; }
float Ball::getY() const { return y_; }
float Ball::getVx() const { return vx_; }
float Ball::getVy() const { return vy_; }
float Ball::getRadius() const { return radius_; }
float Ball::getMass() const { return mass_; }
int Ball::getColor() const { return color_; }
int Ball::getId() const { return id_; }
std::mutex& Ball::getMutex() { return mutex_; }

void Ball::debugLog() const {
    std::cout << "Ball " << id_ << ": pos(" << x_ << ", " << y_
              << ") vel(" << vx_ << ", " << vy_ << ")" << std::endl;
}

