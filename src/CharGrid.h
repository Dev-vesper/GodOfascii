#pragma once
#include "Color.h"
#include <string>
#include <string_view>
#include <vector>

struct Cell {
    char ch = ' ';
    Rgb fg{220, 220, 220};
    Rgb bg{0, 0, 0};
};

// One frame of truecolor character cells, independent of any backend.
class CharGrid {
public:
    void resize(int width, int height);
    int width() const { return width_; }
    int height() const { return height_; }

    void clear();
    void set(int x, int y, char ch, Rgb fg, Rgb bg);
    void setText(int x, int y, std::string_view text, Rgb fg, Rgb bg);
    const Cell& at(int x, int y) const { return cells_[index(x, y)]; }

private:
    int index(int x, int y) const { return y * width_ + x; }

    int width_ = 0;
    int height_ = 0;
    std::vector<Cell> cells_;
};
