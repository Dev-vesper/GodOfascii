#pragma once
#include "FrameBuffer.h"
#include "Map.h"
#include "Minimap.h"
#include "Player.h"
#include "Raycaster.h"
#include "Terminal.h"
#include <chrono>
#include <string>
#include <unordered_map>

// Wires input, simulation and rendering together and runs the main loop.
class Game {
public:
    explicit Game(const std::string& mapPath);
    int run();

private:
    enum class Action {
        MoveForward, MoveBack, StrafeLeft, StrafeRight, TurnLeft, TurnRight,
    };
    using Clock = std::chrono::steady_clock;

    void handleKey(Key key);
    void update(float dt);
    void render();
    void drawHud();

    Map map_;
    Player player_;
    FrameBuffer fb_;
    Raycaster raycaster_;
    Minimap minimap_;
    std::unordered_map<Action, Clock::time_point> held_;
    bool showMinimap_ = true;
    bool running_ = true;
    float fps_ = 0.0f;
};
