#include "game/Game.h"
#include "render/Sprite.h"
#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <thread>

#ifndef _WIN32
#include <fcntl.h>
#include <unistd.h>
#ifdef __FreeBSD__
#include <sys/sysctl.h>
#include <sys/types.h>
#endif
#endif

namespace {
constexpr std::chrono::milliseconds kFrameBudget{16};
constexpr float kMinFov = 40.0f;
constexpr float kMaxFov = 110.0f;
constexpr uint16_t kDefaultPort = 7777;

// "host[:port]" -> host and port; an address without a port means default.
std::string hostOf(const std::string& address) {
    const size_t colon = address.rfind(':');
    return colon == std::string::npos ? address : address.substr(0, colon);
}

uint16_t portOf(const std::string& address) {
    const size_t colon = address.rfind(':');
    if (colon == std::string::npos) return kDefaultPort;
    const int port = std::atoi(address.c_str() + colon + 1);
    return port > 0 && port <= 65535 ? static_cast<uint16_t>(port)
                                     : kDefaultPort;
}
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

bool Game::connect(const std::string& host, uint16_t port) {
    auto net = std::make_unique<NetClient>();
    if (!net->connect(host.c_str(), port)) return false;
    net_ = std::move(net);
    return true;
}

void Game::pollInput() {
    display_->pollInput(input_);
    if (input_.triggered(Action::Quit)) {
        running_ = false;
        return;
    }
    if (startMenu_.active()) {
        handleMenuNav();
        return;  // gameplay shortcuts are unavailable until the game starts
    }
    if (menu_.active()) {
        handleMenuNav();  // Esc maps to Back inside the menu
        return;
    }
    if (input_.triggered(Action::MenuToggle)) menu_.setActive(true);
    if (input_.triggered(Action::ToggleMinimap)) showMinimap_ = !showMinimap_;
    if (input_.triggered(Action::ToggleFullscreen)) display_->toggleFullscreen();
    if (input_.triggered(Action::FovNarrow)) {
        player_.fov = std::max(kMinFov, player_.fov - 5.0f);
    }
    if (input_.triggered(Action::FovWiden)) {
        player_.fov = std::min(kMaxFov, player_.fov + 5.0f);
    }
}

void Game::handleMenuNav() {
    const MenuEvent ev = menuEventFromActions(input_);
    if (startMenu_.active()) {
        // Drain the typed queue first so a last character lands in the
        // address before a same-frame Confirm reads it.
        char c;
        while ((c = input_.popTyped()) != 0) {
            applyMenuCommand(startMenu_.handle(MenuEvent::None, c));
        }
        if (ev != MenuEvent::None) {
            applyMenuCommand(startMenu_.handle(ev, 0));
        }
    } else if (menu_.active() && ev != MenuEvent::None) {
        applyMenuCommand(menu_.handle(ev, 0));
    }
}

void Game::applyMenuCommand(MenuCommand cmd) {
    switch (cmd) {
        case MenuCommand::Resume: menu_.setActive(false); break;
        case MenuCommand::Exit: running_ = false; break;
        case MenuCommand::StartOffline: startMenu_.setActive(false); break;
        case MenuCommand::HostGame: {
            const uint16_t port = portOf(startMenu_.address());
            if (hostGame(port) && joinGame("127.0.0.1", port, 20)) {
                startMenu_.setActive(false);
            } else {
                startMenu_.setStatus("cannot host on port " +
                                     std::to_string(port));
            }
            break;
        }
        case MenuCommand::JoinGame: {
            const std::string host = hostOf(startMenu_.address());
            const uint16_t port = portOf(startMenu_.address());
            if (joinGame(host.empty() ? "127.0.0.1" : host, port, 3)) {
                startMenu_.setActive(false);
            } else {
                startMenu_.setStatus("cannot reach " + startMenu_.address());
            }
            break;
        }
        case MenuCommand::FovDown:
            player_.fov = std::max(kMinFov, player_.fov - 5.0f);
            break;
        case MenuCommand::FovUp:
            player_.fov = std::min(kMaxFov, player_.fov + 5.0f);
            break;
        case MenuCommand::ToggleMinimap: showMinimap_ = !showMinimap_; break;
        case MenuCommand::ToggleFullscreen:
            display_->toggleFullscreen();
            break;
        case MenuCommand::None: break;
    }
}

bool Game::joinGame(const std::string& host, uint16_t port, int attempts) {
    for (int i = 0; i < attempts; ++i) {
        if (connect(host, port)) {
            // The server's spawn keeps joining players off each other.
            if (net_->hasSpawn()) {
                player_.pos = {net_->spawnX(), net_->spawnY()};
            }
            return true;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
    return false;
}

bool Game::hostGame(uint16_t port) {
#ifdef _WIN32
    (void)port;
    return false;  // no fork/exec there (yet); run ascii3d-server by hand
#else
    // The server ships next to this binary; find it through our own path.
    std::string dir = ".";
    char self[4096];
#if defined(__linux__)
    const ssize_t n = readlink("/proc/self/exe", self, sizeof self - 1);
    if (n > 0) {
        self[n] = '\0';
        dir = self;
    }
#elif defined(__FreeBSD__)
    size_t len = sizeof self;
    int mib[4] = {CTL_KERN, KERN_PROC, KERN_PROC_PATHNAME, -1};
    if (sysctl(mib, 4, self, &len, nullptr, 0) == 0) {
        dir = self;
    }
#endif
    const size_t slash = dir.rfind('/');
    const std::string server =
        slash == std::string::npos ? "ascii3d-server"
                                   : dir.substr(0, slash) + "/ascii3d-server";

    const pid_t pid = fork();
    if (pid < 0) return false;
    if (pid == 0) {
        // Detach from the terminal: the game owns the tty, the server must
        // not scribble escape sequences into it.
        setsid();
        const int devnull = open("/dev/null", O_RDWR);
        if (devnull >= 0) {
            dup2(devnull, STDIN_FILENO);
            dup2(devnull, STDOUT_FILENO);
            dup2(devnull, STDERR_FILENO);
            if (devnull > STDERR_FILENO) close(devnull);
        }
        const std::string portArg = "--port=" + std::to_string(port);
        std::vector<char*> argv;
        argv.push_back(const_cast<char*>("ascii3d-server"));
        argv.push_back(const_cast<char*>(portArg.c_str()));
        argv.push_back(nullptr);
        execv(server.c_str(), argv.data());
        _exit(127);
    }
    return true;  // the join retries tell us whether it actually came up
#endif
}

void Game::update(float dt) {
    // While a menu is up the world keeps running, but the player only
    // coasts: no steering input until the menu closes.
    const bool menuUp = menu_.active() || startMenu_.active();
    if (!menuUp) {
        const int mx = input_.mouseDx();
        const int my = input_.mouseDy();
        if (mx != 0 || my != 0) player_.turn(mx, my);
    }
    player_.update(dt, menuUp ? Vec2{} : input_.wish(), map_);
}

void Game::render() {
    const int horizon = player_.horizon(grid_.height());

    raycaster_.render(grid_, map_, player_, depthBuffer_);
    if (net_ && net_->connected()) {
        Sprite::drawPlayers(grid_, net_->others(), player_, depthBuffer_,
                            horizon);
    }

    if (startMenu_.active()) {
        // The title panel sits over the spawn view; HUD and the in-game
        // menu only exist once the game has actually started.
        startMenu_.draw(grid_, static_cast<int>(std::lround(player_.fov)),
                        showMinimap_, display_->fullscreen());
        return;
    }

    // HUD overlays hide behind the menu until it closes again.
    if (!menu_.active()) {
        if (showMinimap_) minimap_.render(grid_, map_, player_);
        hud_.draw(grid_, player_, fps_,
                  net_ && net_->connected() ? net_->others().size() + 1 : 0);
    }

    menu_.draw(grid_, static_cast<int>(std::lround(player_.fov)), showMinimap_,
               display_->fullscreen());
}

int Game::run() {
    if (!map_.loaded()) {
        display_.reset();  // leave the alternate screen so this stays readable
        std::cerr << "error: " << map_.error() << "\n";
        return 1;
    }
    if (!display_->ok()) return 1;  // create() printed the reason
    player_.pos = {map_.spawnX(), map_.spawnY()};
    player_.angle = map_.spawnAngle();
    if (net_) {
        // The server's spawn keeps joining players off each other's cell.
        if (net_->connected() && net_->hasSpawn()) {
            player_.pos = {net_->spawnX(), net_->spawnY()};
        }
        net_->poll();
    } else {
        // No --connect on the command line: the world waits behind the
        // start menu, which decides offline/host/join.
        startMenu_.setActive(true);
    }

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
        if (net_ && net_->connected()) {
            net_->sendState(player_.pos.x, player_.pos.y, player_.angle);
            net_->poll();  // drop or survive a dead server without blocking
        }
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
