#include "game/Player.h"
#include "game/Map.h"
#include <cmath>

namespace {
constexpr float kPi = 3.14159265f;
constexpr float kBodyRadius = 0.2f;
constexpr float kPitchLimit = 0.35f;  // fraction of screen height
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
