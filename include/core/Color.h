#pragma once
#include <cstdint>

struct Rgb {
    uint8_t r = 0;
    uint8_t g = 0;
    uint8_t b = 0;

    constexpr bool operator==(const Rgb& o) const {
        return r == o.r && g == o.g && b == o.b;
    }
    constexpr bool operator!=(const Rgb& o) const { return !(*this == o); }
};

constexpr float clamp01(float v) { return v < 0.0f ? 0.0f : (v > 1.0f ? 1.0f : v); }

constexpr Rgb scale(Rgb c, float f) {
    f = clamp01(f);
    return {static_cast<uint8_t>(c.r * f), static_cast<uint8_t>(c.g * f),
            static_cast<uint8_t>(c.b * f)};
}

constexpr Rgb lerp(Rgb a, Rgb b, float t) {
    t = clamp01(t);
    return {
        static_cast<uint8_t>(a.r + (b.r - a.r) * t),
        static_cast<uint8_t>(a.g + (b.g - a.g) * t),
        static_cast<uint8_t>(a.b + (b.b - a.b) * t),
    };
}
