#include "game/Player.h"
#include "game/Map.h"
#include <algorithm>
#include <cmath>

namespace {
constexpr float kPi = 3.14159265f;
constexpr float kBodyRadius = 0.2f;
constexpr float kPitchLimit = 0.35f;  // fraction of screen height

// Advances one coordinate if the body box would not enter a wall. The other
// coordinate only enters through the two probes at the body's edges.
void slideAxis(float& coord, float other, float d, const Map& map, bool xAxis) {
    const float edge = coord + d + (d > 0.0f ? kBodyRadius : -kBodyRadius);
    const int edgeCell = static_cast<int>(edge);
    const int lo = static_cast<int>(other - kBodyRadius);
    const int hi = static_cast<int>(other + kBodyRadius);
    const bool free = xAxis ? !map.solid(edgeCell, lo) && !map.solid(edgeCell, hi)
                            : !map.solid(lo, edgeCell) && !map.solid(hi, edgeCell);
    if (free) coord += d;
}
}  // namespace

Vec2 Player::plane() const {
    const float halfTan = std::tan(fov * kPi / 180.0f * 0.5f);
    return {-std::sin(angle) * halfTan, std::cos(angle) * halfTan};
}

void Player::turn(int mouseDx, int mouseDy) {
    angle += static_cast<float>(mouseDx) * turnSpeed;
    pitch -= static_cast<float>(mouseDy) * pitchSpeed;
    if (pitch > kPitchLimit) pitch = kPitchLimit;
    if (pitch < -kPitchLimit) pitch = -kPitchLimit;
}

void Player::update(float dt, Vec2 wish, const Map& map) {
    const Vec2 fwd = dir();
    const Vec2 right{-fwd.y, fwd.x};
    const Vec2 accelVec =
        (fwd * wish.y + right * wish.x) * (accel * dt);
    vel_ = vel_ + accelVec;
    // Frame-rate independent exponential friction.
    const float decay = std::exp(-damping * dt);
    vel_ = vel_ * decay;
    if (speed() > maxSpeed) vel_ = normalized(vel_) * maxSpeed;

    if (speed() > 0.001f) move(map, vel_ * dt);

    bobPhase_ += speed() * dt * 2.6f;
}

float Player::headBob() const {
    const float intensity = speed() / maxSpeed;
    return std::sin(bobPhase_) * 1.6f * intensity * intensity;
}

int Player::horizon(int rows) const {
    const int h = static_cast<int>(rows * 0.5f + pitch * rows + headBob());
    return std::clamp(h, rows / 6, rows - rows / 6);
}

void Player::move(const Map& map, Vec2 delta) {
    if (delta.x != 0.0f) slideAxis(pos.x, pos.y, delta.x, map, true);
    if (delta.y != 0.0f) slideAxis(pos.y, pos.x, delta.y, map, false);
}
