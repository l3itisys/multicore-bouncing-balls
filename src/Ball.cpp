#include "Ball.h"
#include <cmath>
#include <algorithm>
#include <thread>

constexpr float GRAVITY = 9.81f;
constexpr float RESTITUTION = 0.9f;

Ball::Ball(int id, float radius, float mass, float x, float y, float vx, float vy, int color)
    : id_(id), radius_(radius), mass_(mass), x_(x), y_(y), vx_(vx), vy_(vy), color_(color),
      running_(false), updateReady_(false), proceed_(false), allBalls_(nullptr) {}

void Ball::start() {
    running_ = true;
    arena_.enqueue([this] { run(); });
}

void Ball::stop() {
    running_ = false;
    proceed_ = true;
}

void Ball::run() {
    while (running_) {
        // Wait for control thread to signal proceed
        while (!proceed_.load(std::memory_order_acquire)) {
            std::this_thread::yield();
        }
        proceed_.store(false, std::memory_order_release);

        // Apply gravity
        vy_.fetch_and_add(GRAVITY * (1.0f / 30.0f));

        // Update position
        updatePosition(1.0f / 30.0f);

        // Check for collisions
        if (allBalls_) {
            checkCollisionWithBoundary(1400.0f, 900.0f); // Screen dimensions hardcoded for simplicity
            checkCollisionWithBalls(*allBalls_);
        }

        // Signal that update is ready
        updateReady_.store(true, std::memory_order_release);
    }
}

void Ball::updatePosition(float dt) {
    x_.fetch_and_add(vx_.load() * dt);
    y_.fetch_and_add(vy_.load() * dt);
}

void Ball::checkCollisionWithBoundary(float screenWidth, float screenHeight) {
    float x = x_.load();
    float y = y_.load();
    float vx = vx_.load();
    float vy = vy_.load();

    bool collision = false;

    if (x - radius_ < 0) {
        x = radius_;
        vx = std::abs(vx) * RESTITUTION;
        collision = true;
    } else if (x + radius_ > screenWidth) {
        x = screenWidth - radius_;
        vx = -std::abs(vx) * RESTITUTION;
        collision = true;
    }

    if (y - radius_ < 0) {
        y = radius_;
        vy = std::abs(vy) * RESTITUTION;
        collision = true;
    } else if (y + radius_ > screenHeight) {
        y = screenHeight - radius_;
        vy = -std::abs(vy) * RESTITUTION;
        collision = true;
    }

    if (collision) {
        x_.store(x);
        y_.store(y);
        vx_.store(vx);
        vy_.store(vy);
    }
}

void Ball::checkCollisionWithBalls(const std::vector<Ball*>& balls) {
    for (Ball* other : balls) {
        if (other->getId() == id_) continue;

        float dx = other->getX() - x_.load();
        float dy = other->getY() - y_.load();
        float distanceSquared = dx * dx + dy * dy;
        float minDist = radius_ + other->getRadius();

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
            float rvx = other->getVx() - vx_.load();
            float rvy = other->getVy() - vy_.load();

            // Velocity along the normal
            float velAlongNormal = rvx * nx + rvy * ny;

            // Do not resolve if velocities are separating
            if (velAlongNormal > 0)
                continue;

            // Calculate impulse scalar
            float e = RESTITUTION;
            float j = -(1 + e) * velAlongNormal;
            j /= (1 / mass_) + (1 / other->getMass());

            // Apply impulse
            float impulsex = j * nx;
            float impulsey = j * ny;

            vx_.fetch_and_sub(impulsex / mass_);
            vy_.fetch_and_sub(impulsey / mass_);

            // For the other ball
            other->vx_.fetch_and_add(impulsex / other->getMass());
            other->vy_.fetch_and_add(impulsey / other->getMass());
        }
    }
}

void Ball::waitForUpdate() {
    while (!updateReady_.load(std::memory_order_acquire)) {
        std::this_thread::yield();
    }
    updateReady_.store(false, std::memory_order_release);
}

void Ball::signalUpdate() {
    proceed_.store(true, std::memory_order_release);
}

float Ball::getX() const {
    return x_.load();
}

float Ball::getY() const {
    return y_.load();
}

float Ball::getRadius() const {
    return radius_;
}

int Ball::getColor() const {
    return color_;
}

int Ball::getId() const {
    return id_;
}

float Ball::getVx() const {
    return vx_.load();
}

float Ball::getVy() const {
    return vy_.load();
}

float Ball::getMass() const {
    return mass_;
}

void Ball::getRenderingData(float& x, float& y, float& radius, int& color) const {
    x = x_.load();
    y = y_.load();
    radius = radius_;
    color = color_;
}

