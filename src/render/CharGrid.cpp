#include "render/CharGrid.h"
#include <algorithm>

void CharGrid::resize(int width, int height) {
    if (width == width_ && height == height_) return;
    width_ = width;
    height_ = height;
    cells_.assign(static_cast<size_t>(width) * height, Cell{});
}

void CharGrid::clear() {
    std::fill(cells_.begin(), cells_.end(), Cell{});
}

void CharGrid::setText(int x, int y, std::string_view text, Rgb fg, Rgb bg) {
    for (char ch : text) {
        if (x >= width_) break;
        set(x, y, ch, fg, bg);
        ++x;
    }
}
