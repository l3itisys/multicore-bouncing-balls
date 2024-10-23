#include "Grid.h"
#include "Ball.h"
#include <cmath>
#include <algorithm>

Grid::Grid(float width, float height, float cellSize)
    : width_(width), height_(height), cellSize_(cellSize) {
    cols_ = static_cast<int>(std::ceil(width_ / cellSize_));
    rows_ = static_cast<int>(std::ceil(height_ / cellSize_));

    cells_.resize(cols_);
    for (int i = 0; i < cols_; ++i) {
        cells_[i].reserve(rows_);
        for (int j = 0; j < rows_; ++j) {
            cells_[i].emplace_back(std::make_unique<Cell>());
        }
    }
}

void Grid::clear() {
    for (auto& column : cells_) {
        for (auto& cell : column) {
            std::lock_guard<std::mutex> lock(cell->mtx);
            cell->balls.clear();
        }
    }
}

void Grid::insertBall(Ball* ball) {
    float x, y;
    ball->getPosition(x, y);

    int cellX = std::clamp(static_cast<int>(x / cellSize_), 0, cols_ - 1);
    int cellY = std::clamp(static_cast<int>(y / cellSize_), 0, rows_ - 1);

    std::lock_guard<std::mutex> lock(cells_[cellX][cellY]->mtx);
    cells_[cellX][cellY]->balls.push_back(ball);
}

void Grid::getPotentialCollisions(Ball* ball, std::vector<Ball*>& potentials) {
    float x, y;
    ball->getPosition(x, y);

    int cellX = static_cast<int>(x / cellSize_);
    int cellY = static_cast<int>(y / cellSize_);

    // Check neighboring cells
    for (int i = -1; i <= 1; ++i) {
        int checkX = cellX + i;
        if (checkX < 0 || checkX >= cols_) continue;

        for (int j = -1; j <= 1; ++j) {
            int checkY = cellY + j;
            if (checkY < 0 || checkY >= rows_) continue;

            std::lock_guard<std::mutex> lock(cells_[checkX][checkY]->mtx);
            for (Ball* other : cells_[checkX][checkY]->balls) {
                if (other != ball) {
                    potentials.push_back(other);
                }
            }
        }
    }
}

