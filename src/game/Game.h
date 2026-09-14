#pragma once
#include "render/CharGrid.h"
#include "Diagnostics.h"
#include "Map.h"
#include "Player.h"
#include "render/Raycaster.h"
#include "platform/Input.h"
#include "platform/Display.h"
#include "ui/Hud.h"
#include "ui/Minimap.h"
#include <chrono>
#include <string>
#include <vector>

// Wires input, simulation and rendering together and runs the main loop.
class Game {
public:
    explicit Game(const std::string& mapPath);
    ~Game();
    int run();

private:
    using Clock = std::chrono::steady_clock;

    void pollInput();
    void update(float dt);
    void render();

    Map map_;
    Player player_;
    std::unique_ptr<Display> display_;
    Input input_;
    Raycaster raycaster_;
    Minimap minimap_;
    Hud hud_;
    CharGrid grid_;
    std::vector<float> depthBuffer_;
    Diagnostics diag_;
    bool debug_ = false;
    bool showMinimap_ = true;
    bool running_ = true;
    float fps_ = 0.0f;
};
