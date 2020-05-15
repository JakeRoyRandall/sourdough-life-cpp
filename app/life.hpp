#pragma once
#include <cstdint>
#include <string>
#include <vector>

class LifeGrid {
public:
    LifeGrid(int width, int height);
    int width() const { return width_; }
    int height() const { return height_; }
    bool alive(int x, int y) const;
    void set(int x, int y, bool value = true);
    void clear();
    void step();
    void run(int steps);
    void seed(uint32_t value, double density = 0.28);
    bool place(const std::string& pattern, int x, int y);
    std::string render() const;

private:
    int width_, height_;
    std::vector<uint8_t> cells_;
    int index(int x, int y) const { return y * width_ + x; }
    int neighbors(int x, int y) const;
};
