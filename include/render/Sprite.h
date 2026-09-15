#pragma once
#include "net/NetClient.h"
#include <vector>

class CharGrid;
class Player;

// Billboard sprite pass: draws the other players into the grid, occluded by
// walls using the per-column depth buffer produced by the raycaster.
namespace Sprite {
void drawPlayers(CharGrid& grid, const std::vector<RemotePlayer>& players,
                 const Player& viewer, const std::vector<float>& depthBuffer,
                 int horizon);
}
