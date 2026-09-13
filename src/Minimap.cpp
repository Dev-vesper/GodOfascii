#include "Minimap.h"
#include "CharGrid.h"
#include "core/Color.h"
#include "Map.h"
#include "Player.h"
#include <algorithm>
#include <cmath>

void Minimap::render(CharGrid& grid, const Map& map, const Player& player) const {
    const int ox = std::max(1, grid.width() - map.width() - 2);
    const int oy = 1;

    for (int y = 0; y < map.height(); ++y) {
        for (int x = 0; x < map.width(); ++x) {
            const uint8_t tile = map.at(x, y);
            const TileDef& def = map.def(tile);
            const Rgb fg = def.solid ? def.color : Rgb{70, 70, 78};
            grid.set(ox + x, oy + y, def.mapChar, fg, {0, 0, 0});
        }
    }
    for (const Vec2& c : map.crystals()) {
        grid.set(ox + static_cast<int>(c.x), oy + static_cast<int>(c.y), '*',
                 {90, 210, 240}, {0, 0, 0});
    }

    const int px = static_cast<int>(player.pos.x);
    const int py = static_cast<int>(player.pos.y);
    grid.set(ox + px, oy + py, '@', {255, 220, 60}, {0, 0, 0});

    const Vec2 d = player.dir();
    grid.set(ox + px + static_cast<int>(std::round(d.x)),
             oy + py + static_cast<int>(std::round(d.y)), 'o', {255, 150, 60},
             {0, 0, 0});
}
