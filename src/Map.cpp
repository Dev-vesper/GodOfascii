#include "Map.h"
#include <fstream>

const TileDef Map::kTiles[] = {
    {' ', false, {0, 0, 0}, '.'},        // 0: floor
    {' ', true, {40, 44, 54}, '+'},      // 1: border (edge of the world)
    {'#', true, {110, 122, 142}, '#'},   // 2: stone wall
    {'%', true, {168, 74, 62}, '%'},     // 3: brick wall
    {'=', true, {92, 128, 88}, '='},     // 4: mossy wall
    {'T', true, {204, 172, 80}, 'T'},    // 5: pillar
};

uint8_t Map::tileFromChar(char ch) {
    switch (ch) {
        case '#': return 2;
        case '%': return 3;
        case '=': return 4;
        case 'T': return 5;
        default: return 0;  // '.', ' ' and anything else are walkable
    }
}

bool Map::load(const std::string& path) {
    std::ifstream in(path);
    if (!in) {
        error_ = "cannot open map file: " + path;
        return false;
    }

    std::vector<std::string> rows;
    std::string line;
    while (std::getline(in, line)) {
        if (!line.empty() && line.back() == '\r') line.pop_back();
        if (line.empty()) continue;
        rows.push_back(line);
        if (static_cast<int>(line.size()) > width_) {
            width_ = static_cast<int>(line.size());
        }
    }
    height_ = static_cast<int>(rows.size());

    if (width_ < 3 || height_ < 3) {
        error_ = "map is too small";
        return false;
    }

    tiles_.assign(static_cast<size_t>(width_) * height_, 0);
    bool spawnFound = false;
    for (int y = 0; y < height_; ++y) {
        for (int x = 0; x < width_; ++x) {
            char ch = x < static_cast<int>(rows[y].size()) ? rows[y][x] : ' ';
            if (ch == '@') {
                spawnX_ = x + 0.5f;
                spawnY_ = y + 0.5f;
                spawnAngle_ = -1.5707963f;  // face north (towards y-)
                spawnFound = true;
                ch = ' ';
            }
            tiles_[static_cast<size_t>(y) * width_ + x] = tileFromChar(ch);
        }
    }

    if (!spawnFound) {
        error_ = "map has no '@' spawn point";
        return false;
    }
    loaded_ = true;
    return true;
}

uint8_t Map::at(int x, int y) const {
    if (x < 0 || y < 0 || x >= width_ || y >= height_) return 1;  // border
    return tiles_[static_cast<size_t>(y) * width_ + x];
}
