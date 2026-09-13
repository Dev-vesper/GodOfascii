#include "FrameBuffer.h"
#include <algorithm>
#include <cstdio>

void FrameBuffer::resize(int width, int height) {
    if (width == width_ && height == height_) return;
    width_ = width;
    height_ = height;
    current_.assign(static_cast<size_t>(width) * height, Cell{});
    previous_.assign(current_.size(), Cell{});
    fullRedraw_ = true;
}

void FrameBuffer::clear() {
    std::fill(current_.begin(), current_.end(), Cell{});
}

void FrameBuffer::set(int x, int y, char ch, uint8_t fg, uint8_t bg) {
    if (x < 0 || y < 0 || x >= width_ || y >= height_) return;
    current_[index(x, y)] = {ch, fg, bg};
}

void FrameBuffer::setText(int x, int y, std::string_view text, uint8_t fg, uint8_t bg) {
    for (char ch : text) {
        if (x >= width_) break;
        set(x, y, ch, fg, bg);
        ++x;
    }
}

void FrameBuffer::flush() {
    std::string out;
    out.reserve(current_.size() * 6);
    if (fullRedraw_) out += "\x1b[2J";

    char esc[32];
    bool colorSet = false;
    uint8_t lastFg = 0;
    uint8_t lastBg = 0;

    for (int y = 0; y < height_; ++y) {
        for (int x = 0; x < width_; ++x) {
            const Cell& c = current_[index(x, y)];
            if (fullRedraw_) {
                if (c == Cell{}) continue;  // left blank by the clear
            } else if (c == previous_[index(x, y)]) {
                continue;
            }

            std::snprintf(esc, sizeof esc, "\x1b[%d;%dH", y + 1, x + 1);
            out += esc;
            if (!colorSet || c.fg != lastFg) {
                std::snprintf(esc, sizeof esc, "\x1b[38;5;%dm", c.fg);
                out += esc;
            }
            if (!colorSet || c.bg != lastBg) {
                std::snprintf(esc, sizeof esc, "\x1b[48;5;%dm", c.bg);
                out += esc;
            }
            colorSet = true;
            lastFg = c.fg;
            lastBg = c.bg;
            out += c.ch;
        }
    }

    std::fwrite(out.data(), 1, out.size(), stdout);
    std::fflush(stdout);
    previous_ = current_;
    fullRedraw_ = false;
}
