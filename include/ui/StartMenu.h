#pragma once
#include "ui/MenuDefs.h"
#include <string>

class CharGrid;

// Startup menu shown before the world takes over: start offline, host a
// local server (dashboard page shows its address while others join), or
// join one by typing its address. Built from the same panel/settings
// pieces as the in-game Menu; owns selection state and the join address
// text only -- everything else comes back as commands.
class StartMenu {
public:
    bool active() const { return active_; }
    void setActive(bool on);

    // Applies a navigation event plus one optional typed character
    // (0 = none, '\b' = backspace; only the address page consumes text)
    // and returns the command it produced.
    MenuCommand handle(MenuEvent ev, char typed);
    const std::string& address() const { return address_; }
    void setStatus(const std::string& s) { status_ = s; }
    // Switches to the host dashboard page showing this address:port.
    void setHosting(const std::string& address) {
        page_ = Page::Host;
        hostAddress_ = address;
        status_.clear();
    }
    void setHostPlayers(int n) { hostPlayers_ = n; }

    void draw(CharGrid& grid, int fov, bool minimapOn, bool fullscreenOn) const;

private:
    enum class Page : uint8_t { Main, Host, Join, Settings };

    bool active_ = false;
    Page page_ = Page::Main;
    int item_ = 0;
    std::string address_ = "127.0.0.1:7777";
    std::string hostAddress_;
    int hostPlayers_ = 0;
    std::string status_;
};
