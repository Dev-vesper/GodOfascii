#include "Game.h"
#include "Sprite.h"
#include <SDL.h>
#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <iostream>
#include <thread>

namespace {
constexpr std::chrono::milliseconds kFrameBudget{16};
constexpr float kMinFov = 40.0f;
constexpr float kMaxFov = 110.0f;
constexpr Rgb kHudFg{235, 235, 235};
constexpr Rgb kHudBg{24, 26, 34};
}  // namespace

Game::Game(const std::string& mapPath) {
    map_.load(mapPath);
}

void Game::handleEvents() {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        switch (event.type) {
            case SDL_QUIT:
                running_ = false;
                break;
            case SDL_KEYDOWN:
                switch (event.key.keysym.sym) {
                    case SDLK_ESCAPE: running_ = false; break;
                    case SDLK_TAB: showMinimap_ = !showMinimap_; break;
                    case SDLK_F11: window_.toggleFullscreen(); break;
                    case SDLK_LEFTBRACKET:
                        player_.fov = std::max(kMinFov, player_.fov - 5.0f);
                        break;
                    case SDLK_RIGHTBRACKET:
                        player_.fov = std::min(kMaxFov, player_.fov + 5.0f);
                        break;
                    default: break;
                }
                break;
            default: break;
        }
    }
}

Vec2 Game::readWish() const {
    const Uint8* keys = SDL_GetKeyboardState(nullptr);
    Vec2 wish{};
    if (keys[SDL_SCANCODE_W] || keys[SDL_SCANCODE_UP]) wish.y += 1.0f;
    if (keys[SDL_SCANCODE_S] || keys[SDL_SCANCODE_DOWN]) wish.y -= 1.0f;
    if (keys[SDL_SCANCODE_D]) wish.x += 1.0f;
    if (keys[SDL_SCANCODE_A]) wish.x -= 1.0f;
    return length(wish) > 0.0f ? normalized(wish) : wish;
}

void Game::update(float dt) {
    int mx = 0;
    int my = 0;
    SDL_GetRelativeMouseState(&mx, &my);
    if (mx != 0 || my != 0) player_.turn(mx, my);

    player_.update(dt, readWish(), map_);

    // Push the player out of crystal sprites.
    for (const Vec2& c : map_.crystals()) {
        const Vec2 d = player_.pos - c;
        const float len = length(d);
        if (len < 0.5f && len > 1e-4f) {
            player_.pos = c + normalized(d) * 0.5f;
        }
    }
}

void Game::render() {
    const int horizon = static_cast<int>(
        grid_.height() * 0.5f + player_.pitch * grid_.height() +
        player_.headBob());

    raycaster_.render(grid_, map_, player_, depthBuffer_);
    Sprite::drawCrystals(grid_, map_, player_, depthBuffer_, horizon);
    if (showMinimap_) minimap_.render(grid_, map_, player_);

    // Crosshair at screen center.
    const int cx = grid_.width() / 2;
    const int cy = grid_.height() / 2;
    grid_.set(cx, cy, '+', {255, 255, 255}, {0, 0, 0});

    drawHud();
    window_.present(grid_);
}

void Game::drawHud() {
    const int y = grid_.height() - 1;
    if (y < 1) return;
    grid_.setText(1, y, "WASD move | mouse look | Tab map | [ ] fov | F11 full | Esc quit",
                  kHudFg, kHudBg);
    char buf[72];
    std::snprintf(buf, sizeof buf, "FPS %d | FOV %d | %dx%d",
                  static_cast<int>(std::lround(fps_)),
                  static_cast<int>(std::lround(player_.fov)), grid_.width(),
                  grid_.height());
    const int x = std::max(1, grid_.width() - static_cast<int>(std::strlen(buf)) - 1);
    grid_.setText(x, y, buf, kHudFg, kHudBg);
}

int Game::run() {
    if (!map_.loaded()) {
        std::cerr << "error: " << map_.error() << "\n";
        return 1;
    }
    if (!window_.ok()) {
        std::cerr << "error: cannot create window: " << SDL_GetError() << "\n";
        return 1;
    }
    player_.pos = {map_.spawnX(), map_.spawnY()};
    player_.angle = map_.spawnAngle();

    auto last = Clock::now();
    while (running_) {
        const auto frameStart = Clock::now();
        float dt = std::chrono::duration<float>(frameStart - last).count();
        last = frameStart;
        dt = std::min(dt, 0.05f);
        if (dt > 1e-4f) {
            fps_ = fps_ == 0.0f ? 1.0f / dt : fps_ * 0.9f + 0.1f / dt;
        }

        handleEvents();

        grid_.resize(window_.cols(), std::max(3, window_.rows() - 1));
        grid_.clear();
        update(dt);
        render();

        const auto spent = Clock::now() - frameStart;
        if (spent < kFrameBudget) {
            std::this_thread::sleep_for(kFrameBudget - spent);
        }
    }
    return 0;
}
