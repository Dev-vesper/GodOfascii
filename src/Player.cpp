#include "Player.h"
#include "Map.h"
#include <cmath>

namespace {
constexpr float kPi = 3.14159265f;
constexpr float kBodyRadius = 0.2f;
}  // namespace

Vec2 Player::plane() const {
    const float halfTan = std::tan(fov * kPi / 180.0f * 0.5f);
    return {-std::sin(angle) * halfTan, std::cos(angle) * halfTan};
}

void Player::move(const Map& map, Vec2 delta) {
    if (delta.x != 0.0f) {
        const float edge =
            pos.x + delta.x + (delta.x > 0.0f ? kBodyRadius : -kBodyRadius);
        if (!map.solid(static_cast<int>(edge), static_cast<int>(pos.y - kBodyRadius)) &&
            !map.solid(static_cast<int>(edge), static_cast<int>(pos.y + kBodyRadius))) {
            pos.x += delta.x;
        }
    }
    if (delta.y != 0.0f) {
        const float edge =
            pos.y + delta.y + (delta.y > 0.0f ? kBodyRadius : -kBodyRadius);
        if (!map.solid(static_cast<int>(pos.x - kBodyRadius), static_cast<int>(edge)) &&
            !map.solid(static_cast<int>(pos.x + kBodyRadius), static_cast<int>(edge))) {
            pos.y += delta.y;
        }
    }
}
