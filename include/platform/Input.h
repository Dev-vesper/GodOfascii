#pragma once
#include "core/Vec2.h"

// One frame of input intent, backend neutral. The active display backend
// fills it via reset() and the setters; the game only reads.
enum class Action {
    Quit,
    ToggleMinimap,
    ToggleFullscreen,
    FovNarrow,
    FovWiden,
};

class Input {
public:
    // Clears all per-frame state. Backends call this before filling.
    void reset() {
        quit_ = minimap_ = fullscreen_ = fovNarrow_ = fovWiden_ = false;
        wish_ = {};
        mouseDx_ = mouseDy_ = 0;
    }

    void setAction(Action action) {
        switch (action) {
            case Action::Quit: quit_ = true; break;
            case Action::ToggleMinimap: minimap_ = true; break;
            case Action::ToggleFullscreen: fullscreen_ = true; break;
            case Action::FovNarrow: fovNarrow_ = true; break;
            case Action::FovWiden: fovWiden_ = true; break;
        }
    }

    void setWish(Vec2 wish) { wish_ = wish; }
    void addMouse(int dx, int dy) {
        mouseDx_ += dx;
        mouseDy_ += dy;
    }

    bool triggered(Action action) const {
        switch (action) {
            case Action::Quit: return quit_;
            case Action::ToggleMinimap: return minimap_;
            case Action::ToggleFullscreen: return fullscreen_;
            case Action::FovNarrow: return fovNarrow_;
            case Action::FovWiden: return fovWiden_;
        }
        return false;
    }

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
