#ifndef GRID_H
#define GRID_H

#include <vector>
#include <memory>
#include <mutex>

class Ball;

class Grid {
public:
    Grid(float width, float height, float cellSize);
    ~Grid() = default;
    Grid(const Grid&) = delete;
    Grid& operator=(const Grid&) = delete;

    void clear();
    void insertBall(Ball* ball);
    void getPotentialCollisions(Ball* ball, std::vector<Ball*>& potentials);

private:
    float width_;
    float height_;
    float cellSize_;
    int cols_;
    int rows_;

    struct Cell {
        std::vector<Ball*> balls;
        std::mutex mtx;
    };

    std::vector<std::vector<std::unique_ptr<Cell>>> cells_;
};

#endif // GRID_H

