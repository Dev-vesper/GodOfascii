#pragma once
#include "Vec2.h"

class Map;

class Player {
public:
    Vec2 pos;
    float angle = 0.0f;      // radians, 0 = +x, grows towards +y (down)
    float fov = 66.0f;       // horizontal field of view, degrees
    float moveSpeed = 3.2f;  // tiles per second
    float turnSpeed = 2.4f;  // radians per second

    Vec2 dir() const { return {std::cos(angle), std::sin(angle)}; }
    Vec2 plane() const;  // camera plane; its length encodes the FOV
    void move(const Map& map, Vec2 delta);  // slides along walls
};
