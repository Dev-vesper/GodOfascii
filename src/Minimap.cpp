#include "Minimap.h"
#include "Map.h"
#include "Player.h"
#include <algorithm>
#include <cmath>

void Minimap::render(FrameBuffer& fb, const Map& map, const Player& player) const {
    const int ox = std::max(1, fb.width() - map.width() - 2);
    const int oy = 1;

    for (int y = 0; y < map.height(); ++y) {
        for (int x = 0; x < map.width(); ++x) {
            const uint8_t tile = map.at(x, y);
            const TileDef& def = map.def(tile);
            const uint8_t fg = def.solid ? rgbTo256(def.color) : 238;
            fb.set(ox + x, oy + y, def.mapChar, fg, 0);
        }
    }

    const int px = static_cast<int>(player.pos.x);
    const int py = static_cast<int>(player.pos.y);
    fb.set(ox + px, oy + py, '@', 226, 0);

    const Vec2 d = player.dir();
    fb.set(ox + px + static_cast<int>(std::round(d.x)),
           oy + py + static_cast<int>(std::round(d.y)), 'o', 214, 0);
}
