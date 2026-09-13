#include "render/Sprite.h"
#include "render/CharGrid.h"
#include "core/Color.h"
#include "Map.h"
#include "Player.h"
#include <cmath>

namespace {
// Glyph art of a crystal, 5 columns x 7 rows.
const char* kCrystalArt[7] = {
    "  ^  ",
    " <o> ",
    "<OOO>",
    "<OXO>",
    "<OOO>",
    " <o> ",
    "  v  ",
};

constexpr float kCrystalSize = 0.7f;  // world height in tiles
constexpr float kFogDensity = 0.075f;
constexpr Rgb kFogColor{16, 20, 30};

Rgb crystalColor(char ch, float fog) {
    Rgb base{120, 220, 255};
    if (ch == 'o') base = {70, 180, 235};
    if (ch == '^' || ch == 'v') base = {200, 250, 255};
    if (ch == 'X') base = {255, 255, 255};
    return lerp(base, kFogColor, fog);
}
}  // namespace

void Sprite::drawCrystals(CharGrid& grid, const Map& map, const Player& player,
                          const std::vector<float>& depthBuffer, int horizon) {
    const int w = grid.width();
    const int h = grid.height();
    const Vec2 dir = player.dir();
    const Vec2 plane = player.plane();
    const float invDet = 1.0f / (plane.x * dir.y - dir.x * plane.y);

    for (const Vec2& sprite : map.crystals()) {
        const Vec2 rel = sprite - player.pos;
        const float transX = invDet * (dir.y * rel.x - dir.x * rel.y);
        const float transY = invDet * (-plane.y * rel.x + plane.x * rel.y);
        if (transY < 0.15f) continue;  // behind the camera

        const int screenX =
            static_cast<int>((w / 2.0f) * (1.0f + transX / transY));
        const float sizePx = std::abs(h / transY);
        const int spriteH = static_cast<int>(sizePx * kCrystalSize);
        const int spriteW = spriteH;
        // Stand on the floor: bottom edge at eye level minus half a tile.
        const int bottom = horizon + static_cast<int>(sizePx * 0.5f);
        const int top = bottom - spriteH;
        const float fog = 1.0f - std::exp(-transY * kFogDensity);

        const int x0 = std::max(0, screenX - spriteW / 2);
        const int x1 = std::min(w - 1, screenX + spriteW / 2);
        for (int x = x0; x <= x1; ++x) {
            if (transY >= depthBuffer[x]) continue;  // behind a wall
            const int texX =
                (x - (screenX - spriteW / 2)) * 5 / std::max(1, spriteW);
            if (texX < 0 || texX > 4) continue;
            for (int row = 0; row < 7; ++row) {
                const int y = top + row * spriteH / 7;
                if (y < 0 || y >= h) continue;
                const char ch = kCrystalArt[row][texX];
                if (ch == ' ') continue;
                grid.set(x, y, ch, crystalColor(ch, fog), kFogColor);
            }
        }
    }
}
