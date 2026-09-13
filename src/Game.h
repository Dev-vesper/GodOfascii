#pragma once
#include "CharGrid.h"
#include "Map.h"
#include "Minimap.h"
#include "Player.h"
#include "Raycaster.h"
#include "Renderer.h"
#include <chrono>
#include <string>
#include <vector>

// Wires SDL events, simulation and rendering together and runs the main loop.
class Game {
public:
    explicit Game(const std::string& mapPath);
    int run();

private:
    using Clock = std::chrono::steady_clock;

    void handleEvent(const SDL_Event& event);
    Vec2 readWish() const;
    void update(float dt);
    void render();
    void drawHud();

    Map map_;
    Player player_;
    Renderer renderer_;
    Raycaster raycaster_;
    Minimap minimap_;
    CharGrid grid_;
    std::vector<float> depthBuffer_;
    bool showMinimap_ = true;
    bool running_ = true;
    float fps_ = 0.0f;
};
