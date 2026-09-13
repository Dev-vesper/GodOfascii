#pragma once

struct CharGrid;
class Player;

// Overlay pass drawn on top of the 3D view: crosshair plus the bottom bar.
class Hud {
public:
    void draw(CharGrid& grid, const Player& player, float fps) const;
};
