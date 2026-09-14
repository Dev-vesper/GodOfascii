#pragma once
class CharGrid;
class Map;
class Player;

// Small top-down overlay of the map with the player marker and crystals.
class Minimap {
public:
    void render(CharGrid& grid, const Map& map, const Player& player) const;
};
