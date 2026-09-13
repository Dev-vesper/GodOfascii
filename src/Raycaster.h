#pragma once
#include "FrameBuffer.h"

class Map;
class Player;

// Grid raycaster (Wolfenstein-style DDA). Renders the first-person view into
// a FrameBuffer; knows nothing about the terminal itself.
class Raycaster {
public:
    explicit Raycaster(FrameBuffer& fb) : fb_(fb) {}

    void render(const Map& map, const Player& player) const;

private:
    FrameBuffer& fb_;
};
