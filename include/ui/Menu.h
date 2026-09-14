#pragma once
#include <cstdint>

struct CharGrid;

// In-game overlay menu: a translucent panel drawn over the live scene. The
// game keeps running while it is open; the other HUD elements hide. Owns
// selection state only -- settings changes come back as commands so the
// game state stays with its owner.
class Menu {
public:
    enum class Command {
        None,
        Resume,  // close the menu
        Exit,    // quit the game
        FovDown,
        FovUp,
        ToggleMinimap,
        ToggleFullscreen,
    };
    enum class Event { Up, Down, Left, Right, Confirm, Back };

    bool active() const { return active_; }
    void setActive(bool on);

    // Applies a navigation event and returns the command it produced.
    Command handle(Event ev);

    // Blends the panel over the rendered frame; scene glyphs stay visible
    // through cells the panel does not use. No-op when inactive. fov,
    // minimapOn and fullscreenOn are display values for the settings page.
    void draw(CharGrid& grid, int fov, bool minimapOn, bool fullscreenOn) const;

private:
    enum class Page : uint8_t { Main, Settings };

    bool active_ = false;
    Page page_ = Page::Main;
    int item_ = 0;
};
