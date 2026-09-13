#pragma once
#include <algorithm>
#include <cstdint>

struct Rgb {
    uint8_t r = 0;
    uint8_t g = 0;
    uint8_t b = 0;
};

constexpr Rgb scale(Rgb c, float f) {
    f = f < 0.0f ? 0.0f : (f > 1.0f ? 1.0f : f);
    return {static_cast<uint8_t>(c.r * f), static_cast<uint8_t>(c.g * f),
            static_cast<uint8_t>(c.b * f)};
}

// Nearest xterm-256 index for an RGB triple (16..231 cube, 232..255 grayscale).
inline uint8_t rgbTo256(Rgb c) {
    int minc = std::min({c.r, c.g, c.b});
    int maxc = std::max({c.r, c.g, c.b});
    if (maxc - minc < 8) {
        int gray = (minc + maxc) / 2;
        return static_cast<uint8_t>(232 + gray * 24 / 256);
    }
    auto step = [](int v) { return v < 48 ? 0 : v < 115 ? 1 : (v - 35) / 40; };
    return static_cast<uint8_t>(16 + 36 * step(c.r) + 6 * step(c.g) + step(c.b));
}
