#pragma once
#include "FrameBuffer.h"

class Map;
class Player;

// Small top-down overlay of the map with the player marker.
class Minimap {
public:
    void render(FrameBuffer& fb, const Map& map, const Player& player) const;
};
