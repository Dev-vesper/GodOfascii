#pragma once
#include "core/Color.h"
#include <string>
#include <string_view>
#include <vector>

struct Cell {
    char ch = ' ';
    Rgb fg{220, 220, 220};
    Rgb bg{0, 0, 0};

    bool operator==(const Cell& o) const {
        return ch == o.ch && fg == o.fg && bg == o.bg;
    }
    bool operator!=(const Cell& o) const { return !(*this == o); }
};

// One frame of truecolor character cells, independent of any backend.
class CharGrid {
public:
    void resize(int width, int height);
    int width() const { return width_; }
    int height() const { return height_; }

    void clear();
    // Inlined: the render loops call this for every cell every frame.
    void set(int x, int y, char ch, Rgb fg, Rgb bg) {
        if (x < 0 || y < 0 || x >= width_ || y >= height_) return;
        cells_[y * width_ + x] = {ch, fg, bg};
    }
    void setText(int x, int y, std::string_view text, Rgb fg, Rgb bg);
    const Cell& at(int x, int y) const { return cells_[y * width_ + x]; }

private:
    int width_ = 0;
    int height_ = 0;
    std::vector<Cell> cells_;
};
