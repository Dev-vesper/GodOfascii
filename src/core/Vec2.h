#pragma once
#include <cmath>

struct Vec2 {
    float x = 0.0f;
    float y = 0.0f;
};

inline Vec2 operator+(Vec2 a, Vec2 b) { return {a.x + b.x, a.y + b.y}; }
inline Vec2 operator-(Vec2 a, Vec2 b) { return {a.x - b.x, a.y - b.y}; }
inline Vec2 operator-(Vec2 v) { return {-v.x, -v.y}; }
inline Vec2 operator*(Vec2 v, float s) { return {v.x * s, v.y * s}; }

inline float length(Vec2 v) { return std::sqrt(v.x * v.x + v.y * v.y); }

inline Vec2 normalized(Vec2 v) {
    float len = length(v);
    return len > 1e-8f ? Vec2{v.x / len, v.y / len} : Vec2{};
}
