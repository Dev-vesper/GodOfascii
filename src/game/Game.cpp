#include "game/Game.h"
#include "render/Sprite.h"
#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdlib>
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
    debug_ = Diagnostics::enabled();
}

Game::~Game() {
    if (!debug_) return;
    const char* path = std::getenv("ASCII3D_STATS");
    diag_.writeSummary(path != nullptr && path[0] != '\0' ? path
                                                         : "/tmp/ascii3d-stats.txt");
}

void Game::pollInput() {
    display_->pollInput(input_);
    if (input_.triggered(Action::Quit)) {
        running_ = false;
        return;
    }
    if (input_.triggered(Action::MenuToggle)) {
        if (menu_.active()) {
            // Esc acts as back: settings page to main, main to close.
            applyMenuCommand(menu_.handle(Menu::Event::Back));
        } else {
            menu_.setActive(true);
        }
    }
    if (menu_.active()) {
        handleMenuInput();
        return;  // gameplay shortcuts are unavailable while the menu is up
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

void Game::handleMenuInput() {
    if (input_.triggered(Action::MenuUp)) {
        applyMenuCommand(menu_.handle(Menu::Event::Up));
    } else if (input_.triggered(Action::MenuDown)) {
        applyMenuCommand(menu_.handle(Menu::Event::Down));
    } else if (input_.triggered(Action::MenuLeft)) {
        applyMenuCommand(menu_.handle(Menu::Event::Left));
    } else if (input_.triggered(Action::MenuRight)) {
        applyMenuCommand(menu_.handle(Menu::Event::Right));
    } else if (input_.triggered(Action::MenuConfirm)) {
        applyMenuCommand(menu_.handle(Menu::Event::Confirm));
    }
}

void Game::applyMenuCommand(Menu::Command cmd) {
    switch (cmd) {
        case Menu::Command::Resume: menu_.setActive(false); break;
        case Menu::Command::Exit: running_ = false; break;
        case Menu::Command::FovDown:
            player_.fov = std::max(kMinFov, player_.fov - 5.0f);
            break;
        case Menu::Command::FovUp:
            player_.fov = std::min(kMaxFov, player_.fov + 5.0f);
            break;
        case Menu::Command::ToggleMinimap: showMinimap_ = !showMinimap_; break;
        case Menu::Command::ToggleFullscreen:
            display_->toggleFullscreen();
            break;
        case Menu::Command::None: break;
    }
}

void Game::update(float dt) {
    const bool menuUp = menu_.active();
    if (!menuUp) {
        const int mx = input_.mouseDx();
        const int my = input_.mouseDy();
        if (mx != 0 || my != 0) player_.turn(mx, my);
    }

    // While the menu is up the world keeps running, but the player only
    // coasts: no steering input until the menu closes.
    player_.update(dt, menuUp ? Vec2{} : input_.wish(), map_);

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
    const int horizon = player_.horizon(grid_.height());

    raycaster_.render(grid_, map_, player_, depthBuffer_);
    Sprite::drawCrystals(grid_, map_, player_, depthBuffer_, horizon);

    // HUD overlays hide behind the menu until it closes again.
    if (!menu_.active()) {
        if (showMinimap_) minimap_.render(grid_, map_, player_);
        hud_.draw(grid_, player_, fps_);
    }

    menu_.draw(grid_, static_cast<int>(std::lround(player_.fov)), showMinimap_,
               display_->fullscreen());
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

        auto t0 = Clock::now();
        update(dt);
        auto t1 = Clock::now();
        render();
        auto t2 = Clock::now();
        if (debug_ && !menu_.active()) {
            diag_.drawOverlay(grid_, static_cast<int>(std::lround(fps_)));
        }
        display_->present(grid_);
        const auto frameEnd = Clock::now();

        if (debug_) {
            using ms = std::chrono::duration<float, std::milli>;
            diag_.recordFrame(dt * 1000.0f);  // true period incl. pacing
            diag_.recordUpdate(ms(t1 - t0).count());
            diag_.recordRender(ms(t2 - t1).count());
            diag_.recordPresent(ms(frameEnd - t2).count(),
                                display_->lastFrameBytes());
        }
        const auto spent = frameEnd - frameStart;
        if (spent < kFrameBudget) {
            std::this_thread::sleep_for(kFrameBudget - spent);
        }
    }
    return 0;
}
