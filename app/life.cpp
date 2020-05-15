#include "life.hpp"
#include <algorithm>
#include <random>
#include <sstream>
#include <stdexcept>

LifeGrid::LifeGrid(int width, int height) : width_(width), height_(height) {
    if (width < 1 || width > 200 || height < 1 || height > 100) throw std::invalid_argument("grid must be 1..200 wide and 1..100 high");
    cells_.assign(static_cast<size_t>(width) * static_cast<size_t>(height), 0);
}

bool LifeGrid::alive(int x, int y) const { return x >= 0 && x < width_ && y >= 0 && y < height_ && cells_[index(x, y)] != 0; }

void LifeGrid::set(int x, int y, bool value) { if (x >= 0 && x < width_ && y >= 0 && y < height_) cells_[index(x, y)] = value ? 1 : 0; }

void LifeGrid::clear() { std::fill(cells_.begin(), cells_.end(), 0); }

int LifeGrid::neighbors(int x, int y) const {
    int count = 0;
    for (int dy = -1; dy <= 1; ++dy) for (int dx = -1; dx <= 1; ++dx) if ((dx || dy) && alive(x + dx, y + dy)) ++count;
    return count;
}

void LifeGrid::step() {
    std::vector<uint8_t> next(cells_.size(), 0);
    for (int y = 0; y < height_; ++y) for (int x = 0; x < width_; ++x) {
        int count = neighbors(x, y); bool staysAlive = alive(x, y) && (count == 2 || count == 3); bool isBorn = !alive(x, y) && count == 3;
        next[index(x, y)] = (staysAlive || isBorn) ? 1 : 0;
    }
    cells_.swap(next);
}

void LifeGrid::run(int steps) { for (int i = 0; i < steps; ++i) step(); }

void LifeGrid::seed(uint32_t value, double density) {
    std::mt19937 generator(value); std::bernoulli_distribution coin(density);
    for (auto& cell : cells_) cell = coin(generator) ? 1 : 0;
}

bool LifeGrid::place(const std::string& pattern, int x, int y) {
    static const std::vector<std::string> block = {"##", "##"};
    static const std::vector<std::string> blinker = {"###"};
    static const std::vector<std::string> glider = {".#.", "..#", "###"};
    const std::vector<std::string>* shape = nullptr;
    if (pattern == "block") shape = &block; else if (pattern == "blinker") shape = &blinker; else if (pattern == "glider") shape = &glider; else return false;
    if (x < 0 || y < 0 || y + static_cast<int>(shape->size()) > height_ || x + static_cast<int>((*shape)[0].size()) > width_) return false;
    clear();
    for (int row = 0; row < static_cast<int>(shape->size()); ++row) for (int col = 0; col < static_cast<int>((*shape)[row].size()); ++col) if ((*shape)[row][col] == '#') set(x + col, y + row);
    return true;
}

std::string LifeGrid::render() const {
    std::ostringstream output;
    for (int y = 0; y < height_; ++y) { for (int x = 0; x < width_; ++x) output << (alive(x, y) ? "██" : "  "); output << '\n'; }
    return output.str();
}
