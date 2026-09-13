#include "Renderer.h"
#include "CharGrid.h"
#include "Font8x8.h"

namespace {
constexpr int kGlyphPx = 8;   // font glyph size
constexpr int kCellPx = 16;   // on-screen cell size (2x scale)
constexpr int kAtlasCols = 16;
}  // namespace

Renderer::Renderer() {
    if (SDL_Init(SDL_INIT_VIDEO) != 0) return;
    window_ = SDL_CreateWindow("ascii3d", SDL_WINDOWPOS_CENTERED,
                               SDL_WINDOWPOS_CENTERED, 1280, 720,
                               SDL_WINDOW_RESIZABLE);
    if (window_ == nullptr) return;
    renderer_ = SDL_CreateRenderer(window_, -1,
                                   SDL_RENDERER_ACCELERATED |
                                       SDL_RENDERER_PRESENTVSYNC);
    if (renderer_ == nullptr) return;
    if (!buildAtlas()) return;
    SDL_SetRelativeMouseMode(SDL_TRUE);
    ok_ = true;
}

Renderer::~Renderer() {
    if (atlas_ != nullptr) SDL_DestroyTexture(atlas_);
    if (renderer_ != nullptr) SDL_DestroyRenderer(renderer_);
    if (window_ != nullptr) SDL_DestroyWindow(window_);
    SDL_Quit();
}

bool Renderer::buildAtlas() {
    SDL_Surface* surf = SDL_CreateRGBSurfaceWithFormat(
        0, kAtlasCols * kGlyphPx, (128 / kAtlasCols) * kGlyphPx, 0,
        SDL_PIXELFORMAT_ARGB8888);
    if (surf == nullptr) return false;

    Uint32* px = static_cast<Uint32*>(surf->pixels);
    const int surfW = surf->w;
    for (int c = 0; c < 128; ++c) {
        const int gx = (c % kAtlasCols) * kGlyphPx;
        const int gy = (c / kAtlasCols) * kGlyphPx;
        for (int row = 0; row < kGlyphPx; ++row) {
            const unsigned char bits = kFont8x8[c][row];
            for (int col = 0; col < kGlyphPx; ++col) {
                px[(gy + row) * surfW + gx + col] =
                    (bits >> col) & 1 ? 0xFFFFFFFFu : 0x00000000u;
            }
        }
    }

    atlas_ = SDL_CreateTextureFromSurface(renderer_, surf);
    SDL_FreeSurface(surf);
    if (atlas_ == nullptr) return false;
    SDL_SetTextureBlendMode(atlas_, SDL_BLENDMODE_BLEND);
    return true;
}

int Renderer::cols() const {
    int w = 0;
    int h = 0;
    SDL_GetRendererOutputSize(renderer_, &w, &h);
    return w / kCellPx;
}

int Renderer::rows() const {
    int w = 0;
    int h = 0;
    SDL_GetRendererOutputSize(renderer_, &w, &h);
    return h / kCellPx;
}

void Renderer::present(const CharGrid& grid) {
    SDL_SetRenderDrawColor(renderer_, 0, 0, 0, 255);
    SDL_RenderClear(renderer_);

    for (int y = 0; y < grid.height(); ++y) {
        for (int x = 0; x < grid.width(); ++x) {
            const Cell& c = grid.at(x, y);
            const SDL_Rect dst{x * kCellPx, y * kCellPx, kCellPx, kCellPx};
            if (c.bg.r != 0 || c.bg.g != 0 || c.bg.b != 0) {
                SDL_SetRenderDrawColor(renderer_, c.bg.r, c.bg.g, c.bg.b, 255);
                SDL_RenderFillRect(renderer_, &dst);
            }
            if (c.ch == ' ') continue;
            const SDL_Rect src{(c.ch % kAtlasCols) * kGlyphPx,
                               (c.ch / kAtlasCols) * kGlyphPx, kGlyphPx,
                               kGlyphPx};
            SDL_SetTextureColorMod(atlas_, c.fg.r, c.fg.g, c.fg.b);
            SDL_RenderCopy(renderer_, atlas_, &src, &dst);
        }
    }
    SDL_RenderPresent(renderer_);
}

void Renderer::toggleFullscreen() {
    const Uint32 flags =
        (SDL_GetWindowFlags(window_) & SDL_WINDOW_FULLSCREEN_DESKTOP) != 0
            ? 0
            : SDL_WINDOW_FULLSCREEN_DESKTOP;
    SDL_SetWindowFullscreen(window_, flags);
}
