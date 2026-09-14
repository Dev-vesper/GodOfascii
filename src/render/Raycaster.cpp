#include "render/Raycaster.h"
#include "render/CharGrid.h"
#include "core/Color.h"
#include "game/Map.h"
#include "game/Player.h"
#include <algorithm>
#include <cmath>
#include <vector>

namespace {
constexpr float kFogDensity = 0.075f;
constexpr Rgb kFogColor{16, 20, 30};
constexpr Rgb kFloorA{56, 50, 42};
constexpr Rgb kFloorB{64, 58, 48};
constexpr Rgb kCeilA{24, 28, 44};
constexpr Rgb kCeilB{28, 32, 50};

// Glyph textures: 8x8 characters plus a per-texel brightness map that also
// darkens the bottom rows like ambient occlusion.
struct WallTex {
    const char* glyphs[8];
    const uint8_t light[8][8];
};

const WallTex kStoneTex = {
    {"########", "###..###", "########", "#..##..#", "########", "###..###",
     "########", "#..##..#"},
    {{255, 250, 250, 245, 245, 250, 250, 255},
     {250, 245, 240, 240, 240, 240, 245, 250},
     {245, 240, 235, 235, 235, 235, 240, 245},
     {240, 235, 220, 220, 220, 220, 235, 240},
     {235, 230, 225, 225, 225, 225, 230, 235},
     {230, 225, 215, 215, 215, 215, 225, 230},
     {225, 220, 210, 210, 210, 210, 220, 225},
     {215, 210, 190, 190, 190, 190, 210, 215}},
};

const WallTex kBrickTex = {
    {"--------", "%%%.%%%%", "%%%.%%%%", "--------", "%%%%.%%%",
     "%%%%.%%%", "--------", "%%%.%%%%"},
    {{200, 190, 190, 185, 185, 190, 190, 200},
     {230, 235, 235, 175, 235, 235, 235, 230},
     {225, 230, 230, 170, 230, 230, 230, 225},
     {195, 185, 185, 180, 180, 185, 185, 195},
     {220, 225, 225, 225, 170, 225, 225, 220},
     {215, 220, 220, 220, 165, 220, 220, 215},
     {190, 180, 180, 175, 175, 180, 180, 190},
     {205, 210, 210, 155, 210, 210, 210, 205}},
};

const WallTex kMossTex = {
    {"========", "=..===.=", "==.=====", "====.===", "=.===..=", "===.====",
     "==.==.==", "========"},
    {{240, 238, 238, 235, 235, 238, 238, 240},
     {238, 205, 205, 232, 230, 205, 232, 238},
     {235, 228, 200, 230, 228, 228, 205, 235},
     {232, 226, 224, 220, 200, 224, 222, 232},
     {230, 198, 222, 220, 218, 205, 202, 230},
     {226, 220, 198, 216, 214, 196, 212, 226},
     {222, 214, 196, 208, 200, 206, 194, 222},
     {210, 204, 194, 190, 188, 190, 190, 210}},
};

const WallTex kPillarTex = {
    {"(======)", "| |||| |", "| |||| |", "| |||| |", "| |||| |",
     "| |||| |", "| |||| |", "'------'"},
    {{235, 255, 255, 255, 255, 255, 255, 235},
     {215, 255, 235, 235, 235, 235, 235, 215},
     {210, 250, 230, 230, 230, 230, 230, 210},
     {205, 245, 225, 225, 225, 225, 225, 205},
     {200, 240, 220, 220, 220, 220, 220, 200},
     {195, 235, 215, 215, 215, 215, 215, 195},
     {190, 230, 210, 210, 210, 210, 210, 190},
     {170, 200, 200, 200, 200, 200, 200, 170}},
};

const WallTex& texFor(uint8_t tile) {
    switch (tile) {
        case 3: return kBrickTex;
        case 4: return kMossTex;
        case 5: return kPillarTex;
        default: return kStoneTex;  // stone and border
    }
}

inline int floorHash(int x, int y, int tx, int ty) {
    return (x * 73856093) ^ (y * 19349663) ^ (tx * 83492791) ^ (ty * 2971215073u);
}
}  // namespace

void Raycaster::render(CharGrid& grid, const Map& map, const Player& player,
                       std::vector<float>& depthBuffer) const {
    const int w = grid.width();
    const int h = grid.height();
    if (w <= 0 || h <= 0) return;
    depthBuffer.assign(w, 1e30f);

    // y-shearing: the horizon moves with pitch and head bob.
    int horizon = static_cast<int>(h * 0.5f + player.pitch * h + player.headBob());
    horizon = std::clamp(horizon, h / 6, h - h / 6);

    const Vec2 dir = player.dir();
    const Vec2 plane = player.plane();

    // --- Walls first: DDA with glyph texturing. Each column records the
    // span it painted so the floor/ceiling pass never overdraws it. ---
    struct Span {
        int top = 0;
        int bottom = -1;
    };
    static std::vector<Span> spans;
    spans.assign(w, Span{0, -1});

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

        int side = 0;
        uint8_t tile = 0;
        while (true) {  // the solid border tile guarantees termination
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

        const float dist = std::max(side == 0 ? sideX - deltaX : sideY - deltaY,
                                    0.05f);
        depthBuffer[x] = dist;

        const int lineH = static_cast<int>(h / dist);
        const int y0 = std::max(0, horizon - lineH / 2);
        const int y1 = std::min(h - 1, horizon + lineH / 2);

        // Texture u coordinate along the wall.
        float wallX = side == 0 ? player.pos.y + dist * ray.y
                                : player.pos.x + dist * ray.x;
        wallX -= std::floor(wallX);
        const int texX = std::clamp(static_cast<int>(wallX * 8.0f), 0, 7);

        const WallTex& tex = texFor(tile);
        const Rgb base = map.def(tile).color;
        const float sideShade = side == 1 ? 0.72f : 1.0f;
        const float fog = 1.0f - std::exp(-dist * kFogDensity);

        for (int y = y0; y <= y1; ++y) {
            int texY = (y - (horizon - lineH / 2)) * 8 / std::max(1, lineH);
            texY = std::clamp(texY, 0, 7);
            const char ch = tex.glyphs[texY][texX];
            const float light = tex.light[texY][texX] / 255.0f * sideShade;
            const Rgb fg = lerp(scale(base, light), kFogColor, fog);
            const Rgb bg = lerp(scale(base, light * 0.35f), kFogColor, fog);
            grid.set(x, y, ch, fg, bg);
        }
        spans[x] = Span{y0, y1};
    }

    // --- Floor and ceiling: perspective-correct row casting, filling only
    // the cells the wall columns did not already cover. ---
    const Vec2 rayDir0 = dir - plane;  // leftmost ray
    const Vec2 rayDir1 = dir + plane;  // rightmost ray
    for (int y = 0; y < h; ++y) {
        const bool isFloor = y > horizon;
        const int p = isFloor ? y - horizon : horizon - y;
        if (p == 0) continue;
        const float rowDist = (0.5f * h) / p;
        const float fog = 1.0f - std::exp(-rowDist * kFogDensity);

        float fx = player.pos.x + rowDist * rayDir0.x;
        float fy = player.pos.y + rowDist * rayDir0.y;
        const float stepX = rowDist * (rayDir1.x - rayDir0.x) / w;
        const float stepY = rowDist * (rayDir1.y - rayDir0.y) / w;

        const Rgb baseA = isFloor ? kFloorA : kCeilA;
        const Rgb baseB = isFloor ? kFloorB : kCeilB;
        const Rgb noiseFg = lerp(scale(baseA, 0.7f), kFogColor, fog);
        const Rgb rowBg[2] = {lerp(baseA, kFogColor, fog),
                              lerp(baseB, kFogColor, fog)};

        for (int x = 0; x < w; ++x) {
            const Span& s = spans[x];
            // Ceiling rows are visible above the wall span, floor rows
            // below it; the wall pass owns everything in between.
            const bool visible =
                isFloor ? y > s.bottom : y < s.top;
            if (visible) {
                const int cellX = static_cast<int>(fx);
                const int cellY = static_cast<int>(fy);
                const int tx = static_cast<int>((fx - cellX) * 4.0f);
                const int ty = static_cast<int>((fy - cellY) * 4.0f);
                const bool alt = ((cellX + cellY) & 1) != 0;

                char ch = ' ';
                const int hash = floorHash(cellX, cellY, tx, ty) & 31;
                if (hash == 0) {
                    ch = isFloor ? '.' : '`';
                } else if (hash == 7) {
                    ch = isFloor ? ',' : '\'';
                }
                grid.set(x, y, ch, noiseFg, rowBg[alt]);
            }
            fx += stepX;
            fy += stepY;
        }
    }
}
