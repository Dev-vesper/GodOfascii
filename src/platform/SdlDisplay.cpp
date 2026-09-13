#include "platform/SdlDisplay.h"
#include "platform/Font8x8.h"
#include "platform/Input.h"
#include "render/CharGrid.h"
#include <SDL.h>

namespace {
constexpr int kGlyphPx = 8;   // font glyph size
constexpr int kCellPx = 16;   // on-screen cell size (2x scale)
constexpr int kAtlasCols = 16;
}  // namespace

struct SdlDisplay::Impl {
    SDL_Window* window = nullptr;
    SDL_Renderer* renderer = nullptr;
    SDL_Texture* atlas = nullptr;
    bool ok = false;

    bool init();
    bool buildAtlas();
};

bool SdlDisplay::Impl::init() {
    if (SDL_Init(SDL_INIT_VIDEO) != 0) return false;
    window = SDL_CreateWindow("ascii3d", SDL_WINDOWPOS_CENTERED,
                              SDL_WINDOWPOS_CENTERED, 1280, 720,
                              SDL_WINDOW_RESIZABLE);
    if (window == nullptr) return false;
    renderer = SDL_CreateRenderer(window, -1,
                                  SDL_RENDERER_ACCELERATED |
                                      SDL_RENDERER_PRESENTVSYNC);
    if (renderer == nullptr) {
        // Software fallback so the game also runs without a GPU driver.
        renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_SOFTWARE);
    }
    if (renderer == nullptr) return false;
    if (!buildAtlas()) return false;
    SDL_SetRelativeMouseMode(SDL_TRUE);
    return true;
}

bool SdlDisplay::Impl::buildAtlas() {
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

    atlas = SDL_CreateTextureFromSurface(renderer, surf);
    SDL_FreeSurface(surf);
    if (atlas == nullptr) return false;
    SDL_SetTextureBlendMode(atlas, SDL_BLENDMODE_BLEND);
    return true;
}

SdlDisplay::SdlDisplay() : impl_(std::make_unique<Impl>()) {
    impl_->ok = impl_->init();
}

SdlDisplay::~SdlDisplay() {
    if (impl_->atlas != nullptr) SDL_DestroyTexture(impl_->atlas);
    if (impl_->renderer != nullptr) SDL_DestroyRenderer(impl_->renderer);
    if (impl_->window != nullptr) SDL_DestroyWindow(impl_->window);
    SDL_Quit();
}

bool SdlDisplay::ok() const { return impl_->ok; }

int SdlDisplay::cols() const {
    int w = 0;
    int h = 0;
    SDL_GetRendererOutputSize(impl_->renderer, &w, &h);
    return w / kCellPx;
}

int SdlDisplay::rows() const {
    int w = 0;
    int h = 0;
    SDL_GetRendererOutputSize(impl_->renderer, &w, &h);
    return h / kCellPx;
}

void SdlDisplay::pollInput(Input& input) {
    input.reset();
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        switch (event.type) {
            case SDL_QUIT:
                input.setAction(Action::Quit);
                break;
            case SDL_KEYDOWN:
                switch (event.key.keysym.sym) {
                    case SDLK_ESCAPE: input.setAction(Action::Quit); break;
                    case SDLK_TAB: input.setAction(Action::ToggleMinimap); break;
                    case SDLK_F11:
                        input.setAction(Action::ToggleFullscreen);
                        break;
                    case SDLK_LEFTBRACKET:
                        input.setAction(Action::FovNarrow);
                        break;
                    case SDLK_RIGHTBRACKET:
                        input.setAction(Action::FovWiden);
                        break;
                    default: break;
                }
                break;
            default: break;
        }
    }

    int mx = 0;
    int my = 0;
    SDL_GetRelativeMouseState(&mx, &my);
    input.addMouse(mx, my);

    const Uint8* keys = SDL_GetKeyboardState(nullptr);
    Vec2 wish{};
    if (keys[SDL_SCANCODE_W] || keys[SDL_SCANCODE_UP]) wish.y += 1.0f;
    if (keys[SDL_SCANCODE_S] || keys[SDL_SCANCODE_DOWN]) wish.y -= 1.0f;
    if (keys[SDL_SCANCODE_D]) wish.x += 1.0f;
    if (keys[SDL_SCANCODE_A]) wish.x -= 1.0f;
    input.setWish(length(wish) > 0.0f ? normalized(wish) : wish);
}

void SdlDisplay::present(const CharGrid& grid) {
    SDL_SetRenderDrawColor(impl_->renderer, 0, 0, 0, 255);
    SDL_RenderClear(impl_->renderer);

    for (int y = 0; y < grid.height(); ++y) {
        for (int x = 0; x < grid.width(); ++x) {
            const Cell& c = grid.at(x, y);
            const SDL_Rect dst{x * kCellPx, y * kCellPx, kCellPx, kCellPx};
            if (c.bg.r != 0 || c.bg.g != 0 || c.bg.b != 0) {
                SDL_SetRenderDrawColor(impl_->renderer, c.bg.r, c.bg.g, c.bg.b,
                                       255);
                SDL_RenderFillRect(impl_->renderer, &dst);
            }
            if (c.ch == ' ') continue;
            const SDL_Rect src{(c.ch % kAtlasCols) * kGlyphPx,
                               (c.ch / kAtlasCols) * kGlyphPx, kGlyphPx,
                               kGlyphPx};
            SDL_SetTextureColorMod(impl_->atlas, c.fg.r, c.fg.g, c.fg.b);
            SDL_RenderCopy(impl_->renderer, impl_->atlas, &src, &dst);
        }
    }
    SDL_RenderPresent(impl_->renderer);
}

void SdlDisplay::toggleFullscreen() {
    const Uint32 flags =
        (SDL_GetWindowFlags(impl_->window) & SDL_WINDOW_FULLSCREEN_DESKTOP) != 0
            ? 0
            : SDL_WINDOW_FULLSCREEN_DESKTOP;
    SDL_SetWindowFullscreen(impl_->window, flags);
}
