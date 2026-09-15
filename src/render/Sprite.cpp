#include "render/Sprite.h"
#include "render/CharGrid.h"
#include "render/Fog.h"
#include "core/Color.h"
#include "game/Player.h"
#include <algorithm>
#include <cmath>

namespace {
// Glyph art of a player figure, 5 columns x 7 rows.
const char* kPlayerArt[7] = {
    "  o  ",
    " /|\\ ",
    " ||| ",
    " ||| ",
    " /|\\ ",
    " / \\ ",
    "/   \\",
};

constexpr float kPlayerSize = 0.8f;  // world height in tiles

// Stable color per session id so each player keeps one identity.
Rgb playerColor(uint32_t id, char ch, float fog) {
    static const Rgb kPalette[8] = {
        {235, 100, 100}, {110, 210, 120}, {240, 190, 90}, {150, 130, 235},
        {235, 130, 200}, {110, 200, 220}, {255, 235, 255}, {120, 170, 255},
    };
    Rgb base = kPalette[id % 8];
    if (ch == 'o' || ch == '|') base = scale(base, 0.8f);
    return lerp(base, kFogColor, fog);
}
}  // namespace

void Sprite::drawPlayers(CharGrid& grid,
                         const std::vector<RemotePlayer>& players,
                         const Player& viewer,
                         const std::vector<float>& depthBuffer, int horizon) {
    const int w = grid.width();
    const int h = grid.height();
    const Vec2 dir = viewer.dir();
    const Vec2 plane = viewer.plane();
    const float invDet = 1.0f / (plane.x * dir.y - dir.x * plane.y);

    for (const RemotePlayer& p : players) {
        const Vec2 rel = {p.x - viewer.pos.x, p.y - viewer.pos.y};
        const float transY = invDet * (-plane.y * rel.x + plane.x * rel.y);
        const float fog = fogFactor(transY);
        if (transY < 0.15f) continue;  // behind the camera
        if (fog > 0.97f) continue;     // fully swallowed by fog
        const float transX = invDet * (dir.y * rel.x - dir.x * rel.y);

        const int screenX =
            static_cast<int>((w / 2.0f) * (1.0f + transX / transY));
        const float sizePx = std::abs(h / transY);
        const int spriteH = static_cast<int>(sizePx * kPlayerSize);
        const int spriteW = spriteH;
        // Stand on the floor: bottom edge at eye level minus half a tile.
        const int bottom = horizon + static_cast<int>(sizePx * 0.5f);
        const int top = bottom - spriteH;

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
                const char ch = kPlayerArt[row][texX];
                if (ch == ' ') continue;
                grid.set(x, y, ch, playerColor(p.id, ch, fog), kFogColor);
            }
        }
    }
}
