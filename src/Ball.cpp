#include "Ball.h"
#include "Grid.h"
#include <cmath>
#include <algorithm>

constexpr float GRAVITY = 9.81f;

Ball::Ball(int id, float radius, float mass, float x, float y, float vx, float vy,
           int color, Grid& grid, float screenWidth, float screenHeight)
    : id_(id), radius_(radius), mass_(mass), x_(x), y_(y), vx_(vx), vy_(vy),
      color_(color), grid_(grid), screenWidth_(screenWidth), screenHeight_(screenHeight) {}

void Ball::applyGravity(float dt) {
    vy_ += GRAVITY * dt;
}

void Ball::updatePosition(float dt) {
    x_ += vx_ * dt;
    y_ += vy_ * dt;
}

void Ball::checkBoundaryCollision() {
    if (x_ - radius_ < 0) {
        x_ = radius_;
        vx_ = std::abs(vx_) * RESTITUTION;
    } else if (x_ + radius_ > screenWidth_) {
        x_ = screenWidth_ - radius_;
        vx_ = -std::abs(vx_) * RESTITUTION;
    }

    if (y_ - radius_ < 0) {
        y_ = radius_;
        vy_ = std::abs(vy_) * RESTITUTION;
    } else if (y_ + radius_ > screenHeight_) {
        y_ = screenHeight_ - radius_;
        vy_ = -std::abs(vy_) * RESTITUTION;
    }
}

void Ball::detectCollisions() {
    std::vector<Ball*> potentialCollisions;
    grid_.getPotentialCollisions(this, potentialCollisions);

    for (Ball* other : potentialCollisions) {
        if (other->getId() == id_) continue;

        // Ensure consistent locking order to prevent deadlocks
        Ball* first = this;
        Ball* second = other;
        if (other->getId() < id_) {
            std::swap(first, second);
        }

        std::scoped_lock lock(first->mtx_, second->mtx_);

        // Perform collision handling
        handleCollision(*other);
    }
}

void Ball::handleCollision(Ball& other) {
    // Locking is handled in detectCollisions()
    float dx = other.x_ - x_;
    float dy = other.y_ - y_;
    float distanceSquared = dx * dx + dy * dy;
    float minDist = radius_ + other.radius_;

    if (distanceSquared < minDist * minDist) {
        float distance = std::sqrt(distanceSquared);
        if (distance == 0.0f) {
            // Avoid division by zero
            dx = minDist;
            dy = 0.0f;
            distance = minDist;
        }
        // Normal vector
        float nx = dx / distance;
        float ny = dy / distance;

        // Relative velocity
        float rvx = other.vx_ - vx_;
        float rvy = other.vy_ - vy_;

        // Velocity along the normal
        float velAlongNormal = rvx * nx + rvy * ny;

        // Do not resolve if velocities are separating
        if (velAlongNormal > 0)
            return;

        // Calculate impulse scalar
        float j = -(1 + RESTITUTION) * velAlongNormal;
        j /= (1 / mass_) + (1 / other.mass_);

        // Apply impulse
        float impulseX = j * nx;
        float impulseY = j * ny;

        vx_ -= impulseX / mass_;
        vy_ -= impulseY / mass_;
        other.vx_ += impulseX / other.mass_;
        other.vy_ += impulseY / other.mass_;

        // Position correction to prevent sinking
        const float percent = 0.8f; // Penetration percentage to correct
        const float slop = 0.05f;   // Penetration allowance
        float correction = std::max(minDist - distance, -slop) / (1 / mass_ + 1 / other.mass_) * percent;

        float correctionX = correction * nx;
        float correctionY = correction * ny;

        x_ -= correctionX / mass_;
        y_ -= correctionY / mass_;
        other.x_ += correctionX / other.mass_;
        other.y_ += correctionY / other.mass_;
    }
}

void Ball::getPosition(float& x, float& y) const {
    std::lock_guard<std::mutex> lock(mtx_);
    x = x_;
    y = y_;
}

float Ball::getVx() const {
    return vx_;
}

float Ball::getVy() const {
    return vy_;
}

