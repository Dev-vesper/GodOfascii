#pragma once
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

struct Cell {
    char ch = ' ';
    uint8_t fg = 250;
    uint8_t bg = 0;

    bool operator==(const Cell& other) const {
        return ch == other.ch && fg == other.fg && bg == other.bg;
    }
};

// Grid of colored character cells with diff-based flushing to stdout.
class FrameBuffer {
public:
    void resize(int width, int height);
    int width() const { return width_; }
    int height() const { return height_; }

    void clear();
    void set(int x, int y, char ch, uint8_t fg, uint8_t bg);
    void setText(int x, int y, std::string_view text, uint8_t fg, uint8_t bg);
    void flush();

private:
    int index(int x, int y) const { return y * width_ + x; }

    int width_ = 0;
    int height_ = 0;
    std::vector<Cell> current_;
    std::vector<Cell> previous_;
    bool fullRedraw_ = true;
};
