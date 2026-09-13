#pragma once
#include "core/Vec2.h"

class Map;

// First-person camera with velocity based movement, mouse look with pitch
// and a subtle head bob while walking.
class Player {
public:
    Vec2 pos;
    float angle = 0.0f;   // yaw in radians, 0 = +x, grows towards +y (down)
    float pitch = 0.0f;   // vertical look, fraction of screen height
    float fov = 70.0f;    // horizontal field of view, degrees

    float maxSpeed = 4.2f;   // tiles per second
    float accel = 30.0f;     // tiles per second^2
    float damping = 8.0f;    // exponential velocity decay per second
    float turnSpeed = 0.0032f;   // radians per mouse pixel
    float pitchSpeed = 0.0022f;  // screen heights per mouse pixel

    Vec2 dir() const { return {std::cos(angle), std::sin(angle)}; }
    Vec2 plane() const;  // camera plane; its length encodes the FOV
    Vec2 velocity() const { return vel_; }
    float speed() const { return length(vel_); }

    void turn(int mouseDx, int mouseDy);
    // wish is a normalized (or zero) direction in camera space: x = strafe,
    // y = forward.
    void update(float dt, Vec2 wish, const Map& map);
    // Vertical view offset in cell rows caused by walking.
    float headBob() const;

private:
    void move(const Map& map, Vec2 delta);

    Vec2 vel_{};
    float bobPhase_ = 0.0f;
};
