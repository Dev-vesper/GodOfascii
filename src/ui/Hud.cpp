#include "ui/Hud.h"
#include "Player.h"
#include "render/CharGrid.h"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

namespace {
constexpr Rgb kHudFg{235, 235, 235};
constexpr Rgb kHudBg{24, 26, 34};
}  // namespace

void Hud::draw(CharGrid& grid, const Player& player, float fps) const {
    // Crosshair at screen center.
    const int cx = grid.width() / 2;
    const int cy = grid.height() / 2;
    grid.set(cx, cy, '+', {255, 255, 255}, {0, 0, 0});

    const int y = grid.height() - 1;
    if (y < 1) return;
    grid.setText(1, y,
                 "WASD move | mouse look | Tab map | [ ] fov | F11 full | Esc quit",
                 kHudFg, kHudBg);
    char buf[72];
    std::snprintf(buf, sizeof buf, "FPS %d | FOV %d | %dx%d",
                  static_cast<int>(std::lround(fps)),
                  static_cast<int>(std::lround(player.fov)), grid.width(),
                  grid.height());
    const int x =
        std::max(1, grid.width() - static_cast<int>(std::strlen(buf)) - 1);
    grid.setText(x, y, buf, kHudFg, kHudBg);
}
