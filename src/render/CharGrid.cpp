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

void CharGrid::set(int x, int y, char ch, Rgb fg, Rgb bg) {
    if (x < 0 || y < 0 || x >= width_ || y >= height_) return;
    cells_[index(x, y)] = {ch, fg, bg};
}

void CharGrid::setText(int x, int y, std::string_view text, Rgb fg, Rgb bg) {
    for (char ch : text) {
        if (x >= width_) break;
        set(x, y, ch, fg, bg);
        ++x;
    }
}
