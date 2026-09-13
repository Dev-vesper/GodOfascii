#pragma once
#include "core/Vec2.h"

// Translates raw SDL events and device state into game intents, one snapshot
// per frame. Game code never touches SDL input APIs directly.
enum class Action {
    Quit,
    ToggleMinimap,
    ToggleFullscreen,
    FovNarrow,
    FovWiden,
};

class Input {
public:
    // Pumps the event queue and refreshes all state; call once per frame.
    void beginFrame();

    bool triggered(Action action) const;
    // Normalized movement intent in camera space: x = strafe, y = forward.
    Vec2 wish() const { return wish_; }
    int mouseDx() const { return mouseDx_; }
    int mouseDy() const { return mouseDy_; }

private:
    bool quit_ = false;
    bool minimap_ = false;
    bool fullscreen_ = false;
    bool fovNarrow_ = false;
    bool fovWiden_ = false;
    Vec2 wish_{};
    int mouseDx_ = 0;
    int mouseDy_ = 0;
};
