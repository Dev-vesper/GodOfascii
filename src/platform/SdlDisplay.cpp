#include "platform/SdlDisplay.h"
#include "platform/Font8x8.h"
#include "platform/Input.h"
#include "render/CharGrid.h"
#include <SDL.h>

namespace {
constexpr int kGlyphPx = 8;   // font glyph size
constexpr int kCellPx = 16;   // on-screen cell size (2x scale)
}  // namespace

struct SdlDisplay::Impl {
    SDL_Window* window = nullptr;
    SDL_Renderer* renderer = nullptr;
    SDL_Texture* frame = nullptr;  // one streaming texture per frame
    std::vector<Uint32> pixels;
    int texW = 0;
    int texH = 0;
    size_t lastBytes = 0;
    bool ok = false;

    bool init();
    bool ensureTexture(int gridW, int gridH);
    void blitCell(const Cell& c, int cx, int cy);
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
    SDL_SetRelativeMouseMode(SDL_TRUE);
    return true;
}

bool SdlDisplay::Impl::ensureTexture(int gridW, int gridH) {
    const int w = gridW * kCellPx;
    const int h = gridH * kCellPx;
    if (frame != nullptr && w == texW && h == texH) return true;
    if (frame != nullptr) SDL_DestroyTexture(frame);
    frame = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888,
                              SDL_TEXTUREACCESS_STREAMING, w, h);
    if (frame == nullptr) return false;
    texW = w;
    texH = h;
    pixels.assign(static_cast<size_t>(w) * h, 0xFF000000u);
    return true;
}

void SdlDisplay::Impl::blitCell(const Cell& c, int cx, int cy) {
    const Uint32 bg = 0xFF000000u | (c.bg.b << 16) | (c.bg.g << 8) | c.bg.r;
    const Uint32 fg = 0xFF000000u | (c.fg.b << 16) | (c.fg.g << 8) | c.fg.r;
    const int px0 = cx * kCellPx;
    const int py0 = cy * kCellPx;
    const int scale = kCellPx / kGlyphPx;
    const unsigned char* glyph = kFont8x8[static_cast<unsigned char>(c.ch)];
    for (int row = 0; row < kGlyphPx; ++row) {
        const unsigned char bits = glyph[row];
        for (int sy = 0; sy < scale; ++sy) {
            Uint32* dst = &pixels[(py0 + row * scale + sy) * texW + px0];
            for (int col = 0; col < kGlyphPx; ++col) {
                const Uint32 px = ((bits >> col) & 1) != 0 ? fg : bg;
                for (int sx = 0; sx < scale; ++sx) dst[sx] = px;
                dst += scale;
            }
        }
    }
}

SdlDisplay::SdlDisplay() : impl_(std::make_unique<Impl>()) {
    impl_->ok = impl_->init();
}

SdlDisplay::~SdlDisplay() {
    if (impl_->frame != nullptr) SDL_DestroyTexture(impl_->frame);
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
    if (!impl_->ensureTexture(grid.width(), grid.height())) return;
    for (int y = 0; y < grid.height(); ++y) {
        for (int x = 0; x < grid.width(); ++x) {
            impl_->blitCell(grid.at(x, y), x, y);
        }
    }
    SDL_UpdateTexture(impl_->frame, nullptr, impl_->pixels.data(),
                      impl_->texW * sizeof(Uint32));
    SDL_RenderCopy(impl_->renderer, impl_->frame, nullptr, nullptr);
    SDL_RenderPresent(impl_->renderer);
    impl_->lastBytes = static_cast<size_t>(impl_->texW) * impl_->texH * 4;
}

void SdlDisplay::toggleFullscreen() {
    const Uint32 flags =
        (SDL_GetWindowFlags(impl_->window) & SDL_WINDOW_FULLSCREEN_DESKTOP) != 0
            ? 0
            : SDL_WINDOW_FULLSCREEN_DESKTOP;
    SDL_SetWindowFullscreen(impl_->window, flags);
}
