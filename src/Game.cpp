#include "Game.h"
#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <iostream>
#include <thread>

namespace {
// Terminals repeat held keys instead of reporting release, so an action counts
// as held while its key keeps arriving within this window.
constexpr auto kHoldWindow = std::chrono::milliseconds(250);
constexpr std::chrono::milliseconds kFrameBudget{16};
constexpr float kMinFov = 30.0f;
constexpr float kMaxFov = 120.0f;
}  // namespace

Game::Game(const std::string& mapPath) : raycaster_(fb_) {
    map_.load(mapPath);
}

void Game::handleKey(Key key) {
    const auto now = Clock::now();
    switch (key) {
        case Key::W: case Key::Up:    held_[Action::MoveForward] = now; break;
        case Key::S: case Key::Down:  held_[Action::MoveBack] = now; break;
        case Key::A:                  held_[Action::StrafeLeft] = now; break;
        case Key::D:                  held_[Action::StrafeRight] = now; break;
        case Key::Q: case Key::Left:  held_[Action::TurnLeft] = now; break;
        case Key::E: case Key::Right: held_[Action::TurnRight] = now; break;
        case Key::Tab:      showMinimap_ = !showMinimap_; break;
        case Key::LBracket: player_.fov = std::max(kMinFov, player_.fov - 5.0f); break;
        case Key::RBracket: player_.fov = std::min(kMaxFov, player_.fov + 5.0f); break;
        case Key::Escape:   running_ = false; break;
        default: break;
    }
}

void Game::update(float dt) {
    const auto now = Clock::now();
    auto active = [&](Action a) {
        auto it = held_.find(a);
        return it != held_.end() && now - it->second < kHoldWindow;
    };

    const Vec2 fwd = player_.dir();
    const Vec2 right{-fwd.y, fwd.x};
    Vec2 move{};
    if (active(Action::MoveForward)) move = move + fwd;
    if (active(Action::MoveBack))    move = move - fwd;
    if (active(Action::StrafeRight)) move = move + right;
    if (active(Action::StrafeLeft))  move = move - right;
    if (length(move) > 0.0f) {
        player_.move(map_, normalized(move) * player_.moveSpeed * dt);
    }

    float turn = 0.0f;
    if (active(Action::TurnRight)) turn += 1.0f;
    if (active(Action::TurnLeft))  turn -= 1.0f;
    player_.angle += turn * player_.turnSpeed * dt;
}

void Game::render() {
    fb_.clear();
    raycaster_.render(map_, player_);
    if (showMinimap_) minimap_.render(fb_, map_, player_);
    drawHud();
    fb_.flush();
}

void Game::drawHud() {
    const int y = fb_.height() - 1;
    if (y < 1) return;
    fb_.setText(1, y, "WASD move | QE turn | Tab map | [ ] fov | Esc quit",
                252, 236);
    char buf[64];
    std::snprintf(buf, sizeof buf, "FPS %d | FOV %d | %dx%d",
                  static_cast<int>(std::lround(fps_)),
                  static_cast<int>(std::lround(player_.fov)),
                  fb_.width(), fb_.height());
    const int x = std::max(1, fb_.width() - static_cast<int>(std::strlen(buf)) - 1);
    fb_.setText(x, y, buf, 252, 236);
}

int Game::run() {
    if (!map_.loaded()) {
        std::cerr << "error: " << map_.error() << "\n";
        return 1;
    }
    player_.pos = {map_.spawnX(), map_.spawnY()};
    player_.angle = map_.spawnAngle();

    Terminal term;
    auto last = Clock::now();
    while (running_) {
        const auto frameStart = Clock::now();
        float dt = std::chrono::duration<float>(frameStart - last).count();
        last = frameStart;
        dt = std::min(dt, 0.1f);
        if (dt > 1e-4f) {
            fps_ = fps_ == 0.0f ? 1.0f / dt : fps_ * 0.9f + 0.1f / dt;
        }

        fb_.resize(term.width(), std::max(3, term.height() - 1));

        for (;;) {
            const Key key = term.readKey();
            if (key == Key::None) break;
            handleKey(key);
        }
        update(dt);
        render();

        const auto spent = Clock::now() - frameStart;
        if (spent < kFrameBudget) {
            std::this_thread::sleep_for(kFrameBudget - spent);
        }
    }
    return 0;
}
