#include "Game.h"
#include "render/Sprite.h"
#include <algorithm>
#include <chrono>
#include <iostream>
#include <thread>

namespace {
constexpr std::chrono::milliseconds kFrameBudget{16};
constexpr float kMinFov = 40.0f;
constexpr float kMaxFov = 110.0f;
}  // namespace

Game::Game(const std::string& mapPath) {
    map_.load(mapPath);
    display_ = Display::create();
}

void Game::pollInput() {
    display_->pollInput(input_);
    if (input_.triggered(Action::Quit)) {
        running_ = false;
        return;
    }
    if (input_.triggered(Action::ToggleMinimap)) showMinimap_ = !showMinimap_;
    if (input_.triggered(Action::ToggleFullscreen)) display_->toggleFullscreen();
    if (input_.triggered(Action::FovNarrow)) {
        player_.fov = std::max(kMinFov, player_.fov - 5.0f);
    }
    if (input_.triggered(Action::FovWiden)) {
        player_.fov = std::min(kMaxFov, player_.fov + 5.0f);
    }
}

void Game::update(float dt) {
    const int mx = input_.mouseDx();
    const int my = input_.mouseDy();
    if (mx != 0 || my != 0) player_.turn(mx, my);

    player_.update(dt, input_.wish(), map_);

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

    hud_.draw(grid_, player_, fps_);
    display_->present(grid_);
}

int Game::run() {
    if (!map_.loaded()) {
        std::cerr << "error: " << map_.error() << "\n";
        return 1;
    }
    if (!display_->ok()) return 1;  // create() printed the reason
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

        pollInput();

        grid_.resize(display_->cols(), std::max(3, display_->rows() - 1));
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
