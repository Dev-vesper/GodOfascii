#pragma once
#include "core/Vec2.h"
#include <vector>

class CharGrid;
class Map;
class Player;

// Billboard sprite pass: draws crystal entities into the grid, occluded by
// walls using the per-column depth buffer produced by the raycaster.
namespace Sprite {
void drawCrystals(CharGrid& grid, const Map& map, const Player& player,
                  const std::vector<float>& depthBuffer, int horizon);
}
