#pragma once
#include "core/Color.h"
#include <cmath>

// Distance fog shared by the wall, floor and sprite passes.
constexpr float kFogDensity = 0.075f;
constexpr Rgb kFogColor{16, 20, 30};

inline float fogFactor(float dist) {
    return 1.0f - std::exp(-dist * kFogDensity);
}
