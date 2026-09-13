#pragma once
#include "core/Color.h"
#include "core/Vec2.h"
#include <cstdint>
#include <string>
#include <vector>

struct TileDef {
    char fileChar;  // character used in map files
    bool solid;
    Rgb color;      // tint used by the renderer
    char mapChar;   // character shown on the minimap
};

// Tile map loaded from a plain text file. Each character is one tile;
// out-of-bounds tiles are a solid border, so the world is always closed.
// '*' marks a crystal sprite entity on a walkable tile.
class Map {
public:
    bool load(const std::string& path);
    bool loaded() const { return loaded_; }
    const std::string& error() const { return error_; }

    int width() const { return width_; }
    int height() const { return height_; }

    uint8_t at(int x, int y) const;
    bool solid(int x, int y) const { return def(at(x, y)).solid; }
    const TileDef& def(uint8_t tile) const { return kTiles[tile]; }

    float spawnX() const { return spawnX_; }
    float spawnY() const { return spawnY_; }
    float spawnAngle() const { return spawnAngle_; }

    const std::vector<Vec2>& crystals() const { return crystals_; }

private:
    static const TileDef kTiles[];
    static uint8_t tileFromChar(char ch);

    int width_ = 0;
    int height_ = 0;
    std::vector<uint8_t> tiles_;
    std::vector<Vec2> crystals_;
    float spawnX_ = 1.5f;
    float spawnY_ = 1.5f;
    float spawnAngle_ = 0.0f;
    bool loaded_ = false;
    std::string error_;
};
