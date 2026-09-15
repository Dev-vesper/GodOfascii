#pragma once

class CharGrid;
class Player;

// Overlay pass drawn on top of the 3D view: crosshair plus the bottom bar.
class Hud {
public:
    // onlinePlayers is 0 offline, otherwise everyone including ourselves.
    void draw(CharGrid& grid, const Player& player, float fps,
              int onlinePlayers = 0) const;
};
