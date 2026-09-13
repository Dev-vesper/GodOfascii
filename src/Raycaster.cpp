#include "Raycaster.h"
#include "Map.h"
#include "Player.h"
#include <algorithm>
#include <cmath>
#include <vector>

namespace {
const char kShadeRamp[] = " .:-=+*#%@";
constexpr int kRampLast = 9;

char rampChar(float shade) {
    const int i = static_cast<int>(shade * kRampLast + 0.5f);
    return kShadeRamp[std::clamp(i, 0, kRampLast)];
}
}  // namespace

void Raycaster::render(const Map& map, const Player& player) const {
    const int w = fb_.width();
    const int h = fb_.height();
    if (w <= 0 || h <= 0) return;

    // Distance-shaded sky and floor: one color per row.
    std::vector<uint8_t> ceiling(h);
    std::vector<uint8_t> floorRow(h);
    for (int y = 0; y < h; ++y) {
        float t = std::abs(y - h * 0.5f) / (h * 0.5f);  // 0 at horizon
        t = std::clamp(t, 0.0f, 1.0f);
        ceiling[y] = rgbTo256(scale(Rgb{28, 34, 50}, 0.25f + 0.75f * t));
        floorRow[y] = rgbTo256(scale(Rgb{66, 58, 48}, 0.2f + 0.8f * t));
    }

    const Vec2 dir = player.dir();
    const Vec2 plane = player.plane();

    for (int x = 0; x < w; ++x) {
        const float cameraX = 2.0f * x / static_cast<float>(w) - 1.0f;
        const Vec2 ray = dir + plane * cameraX;

        int mapX = static_cast<int>(player.pos.x);
        int mapY = static_cast<int>(player.pos.y);
        const float deltaX = ray.x == 0.0f ? 1e30f : std::abs(1.0f / ray.x);
        const float deltaY = ray.y == 0.0f ? 1e30f : std::abs(1.0f / ray.y);

        int stepX = 0;
        int stepY = 0;
        float sideX = 0.0f;
        float sideY = 0.0f;
        if (ray.x < 0.0f) {
            stepX = -1;
            sideX = (player.pos.x - mapX) * deltaX;
        } else {
            stepX = 1;
            sideX = (mapX + 1.0f - player.pos.x) * deltaX;
        }
        if (ray.y < 0.0f) {
            stepY = -1;
            sideY = (player.pos.y - mapY) * deltaY;
        } else {
            stepY = 1;
            sideY = (mapY + 1.0f - player.pos.y) * deltaY;
        }

        // DDA walk; the solid border tile guarantees termination.
        int side = 0;
        uint8_t tile = 0;
        while (true) {
            if (sideX < sideY) {
                sideX += deltaX;
                mapX += stepX;
                side = 0;
            } else {
                sideY += deltaY;
                mapY += stepY;
                side = 1;
            }
            tile = map.at(mapX, mapY);
            if (map.solid(mapX, mapY)) break;
        }

        float dist = side == 0 ? sideX - deltaX : sideY - deltaY;
        dist = std::max(dist, 0.05f);

        const int lineH = static_cast<int>(h / dist);
        const int y0 = std::max(0, h / 2 - lineH / 2);
        const int y1 = std::min(h - 1, h / 2 + lineH / 2);

        float shade = 1.0f / (1.0f + dist * dist * 0.045f);
        if (side == 1) shade *= 0.72f;  // darker on north/south faces
        shade = std::clamp(shade, 0.06f, 1.0f);

        const Rgb col = scale(map.def(tile).color, shade);
        const uint8_t fg = rgbTo256(col);
        const uint8_t bg = rgbTo256(scale(col, 0.3f));
        const char ch = rampChar(shade);

        for (int y = 0; y < y0; ++y) fb_.set(x, y, ' ', ceiling[y], ceiling[y]);
        for (int y = y0; y <= y1; ++y) fb_.set(x, y, ch, fg, bg);
        for (int y = y1 + 1; y < h; ++y) fb_.set(x, y, ' ', floorRow[y], floorRow[y]);
    }
}
