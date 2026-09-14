#pragma once
#include <vector>

class CharGrid;
class Map;
class Player;

// Grid raycaster (Wolfenstein-style DDA) with perspective-correct floor and
// ceiling casting, glyph wall textures, y-shearing pitch and distance fog.
// Writes the frame into a CharGrid and a per-column depth buffer for sprites.
class Raycaster {
public:
    void render(CharGrid& grid, const Map& map, const Player& player,
                std::vector<float>& depthBuffer) const;
};
