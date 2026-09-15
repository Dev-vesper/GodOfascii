#pragma once
#include "ui/MenuDefs.h"

class CharGrid;

// In-game overlay menu: a translucent panel drawn over the live scene. The
// game keeps running while it is open; the other HUD elements hide. Owns
// selection state only -- settings changes come back as commands so the
// game state stays with its owner.
class Menu {
public:
    bool active() const { return active_; }
    void setActive(bool on);

    // Applies a navigation event (an optional typed character is accepted
    // for symmetry; this menu has no text entry) and returns the command
    // it produced.
    MenuCommand handle(MenuEvent ev, char typed = 0);

    // Blends the panel over the rendered frame. fov, minimapOn and
    // fullscreenOn are display values for the settings page.
    void draw(CharGrid& grid, int fov, bool minimapOn, bool fullscreenOn) const;

private:
    enum class Page : uint8_t { Main, Settings };

    bool active_ = false;
    Page page_ = Page::Main;
    int item_ = 0;
};
