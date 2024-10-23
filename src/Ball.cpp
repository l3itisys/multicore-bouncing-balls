#include "Ball.h"
#include "Grid.h"
#include <cmath>
#include <algorithm>

constexpr float GRAVITY = 400.0f;
constexpr float RESTITUTION = 0.8f;

Ball::Ball(int id, float radius, float mass, float x, float y, float vx, float vy,
           int color, Grid& grid, float screenWidth, float screenHeight)
    : id_(id), radius_(radius), mass_(mass), x_(x), y_(y), vx_(vx), vy_(vy),
      color_(color), grid_(grid), screenWidth_(screenWidth), screenHeight_(screenHeight) {}

void Ball::updatePosition(float dt) {
    vy_ += GRAVITY * dt;

    float newX = x_ + vx_ * dt;
    float newY = y_ + vy_ * dt;

    if (newX - radius_ < 0) {
        newX = radius_;
        vx_ = std::abs(vx_) * RESTITUTION;
    } else if (newX + radius_ > screenWidth_) {
        newX = screenWidth_ - radius_;
        vx_ = -std::abs(vx_) * RESTITUTION;
    }

    if (newY - radius_ < 0) {
        newY = radius_;
        vy_ = std::abs(vy_) * RESTITUTION;
    } else if (newY + radius_ > screenHeight_) {
        newY = screenHeight_ - radius_;
        vy_ = -std::abs(vy_) * RESTITUTION;
    }

    x_ = newX;
    y_ = newY;

    const float DAMPING = 0.999f;
    vx_ *= DAMPING;
    vy_ *= DAMPING;
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

        float dx = other->x_ - x_;
        float dy = other->y_ - y_;
        float distanceSquared = dx * dx + dy * dy;
        float minDist = radius_ + other->radius_;

        if (distanceSquared < minDist * minDist) {
            float distance = std::sqrt(distanceSquared);
            if (distance == 0.0f) {
                // Avoid division by zero
                dx = minDist;
                dy = 0.0f;
                distance = minDist;
            }
            float nx = dx / distance;
            float ny = dy / distance;

            float relativeVx = vx_ - other->vx_;
            float relativeVy = vy_ - other->vy_;
            float relativeSpeed = relativeVx * nx + relativeVy * ny;

            if (relativeSpeed < 0) {
                // Calculate impulse scalar
                float e = RESTITUTION;
                float j = -(1 + e) * relativeSpeed;
                j /= (1 / mass_) + (1 / other->mass_);

                // Apply impulse to both balls
                float impulseX = j * nx;
                float impulseY = j * ny;

                vx_ += impulseX / mass_;
                vy_ += impulseY / mass_;
                other->vx_ -= impulseX / other->mass_;
                other->vy_ -= impulseY / other->mass_;

                // Correct positions to prevent overlapping
                float overlap = (minDist - distance) / 2.0f;
                x_ -= nx * overlap;
                y_ -= ny * overlap;
                other->x_ += nx * overlap;
                other->y_ += ny * overlap;
            }
        }
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

