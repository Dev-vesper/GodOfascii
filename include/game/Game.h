#pragma once
#include "render/CharGrid.h"
#include "game/Diagnostics.h"
#include "game/Map.h"
#include "game/Player.h"
#include "net/NetClient.h"
#include "render/Raycaster.h"
#include "platform/Input.h"
#include "platform/Display.h"
#include "ui/Hud.h"
#include "ui/Minimap.h"
#include "ui/Menu.h"
#include "ui/StartMenu.h"
#include <chrono>
#include <memory>
#include <string>
#include <vector>

// Wires input, simulation and rendering together and runs the main loop.
// With a NetClient attached the game additionally reports the player's
// position to the server and draws the other players.
class Game {
public:
    explicit Game(const std::string& mapPath);
    ~Game();
    // Connects to an online session; returns false when unreachable (the
    // game still runs offline).
    bool connect(const std::string& host, uint16_t port);
    int run();

private:
    using Clock = std::chrono::steady_clock;

    void pollInput();
    // Feeds the open menu (start menu or in-game menu) this frame's nav
    // event and typed character, then applies the resulting command.
    void handleMenuNav();
    void applyMenuCommand(MenuCommand cmd);
    void update(float dt);
    void render();
    // Connects to host:port, retrying a few times so a freshly spawned
    // server has time to bind. Applies the server spawn on success.
    bool joinGame(const std::string& host, uint16_t port, int attempts);
    // Spawns the sibling ascii3d-server binary and returns its success.
    bool hostGame(uint16_t port);

    Map map_;
    Player player_;
    std::unique_ptr<Display> display_;
    Input input_;
    std::unique_ptr<NetClient> net_;
    Raycaster raycaster_;
    Minimap minimap_;
    Hud hud_;
    Menu menu_;
    StartMenu startMenu_;
    CharGrid grid_;
    std::vector<float> depthBuffer_;
    Diagnostics diag_;
    bool debug_ = false;
    bool showMinimap_ = true;
    bool running_ = true;
    float fps_ = 0.0f;
};
