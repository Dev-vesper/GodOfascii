#pragma once
#include "core/Vec2.h"
#include <cstdint>
#include <string>

// One frame of input intent, backend neutral. The active display backend
// fills it via reset() and the setters; the game only reads.
enum class Action : uint16_t {
    Quit = 1u << 0,
    ToggleMinimap = 1u << 1,
    ToggleFullscreen = 1u << 2,
    FovNarrow = 1u << 3,
    FovWiden = 1u << 4,
    MenuToggle = 1u << 5,
    MenuUp = 1u << 6,
    MenuDown = 1u << 7,
    MenuLeft = 1u << 8,
    MenuRight = 1u << 9,
    MenuConfirm = 1u << 10,
};

class Input {
public:
    // Clears all per-frame state. Backends call this before filling.
    void reset() {
        actions_ = 0;
        wish_ = {};
        mouseDx_ = mouseDy_ = 0;
        typed_.clear();
    }

    void setAction(Action action) { actions_ |= static_cast<uint16_t>(action); }
    bool triggered(Action action) const {
        return (actions_ & static_cast<uint16_t>(action)) != 0;
    }

    void setWish(Vec2 wish) { wish_ = wish; }
    void addMouse(int dx, int dy) {
        mouseDx_ += dx;
        mouseDy_ += dy;
    }
    // Text entry queue for menus: printable bytes plus '\b' backspaces,
    // capped so a pasted burst cannot grow without bound. Backends push
    // every byte they saw this frame; menus drain them with popTyped().
    void typeChar(char c) {
        if (typed_.size() < 32) typed_ += c;
    }
    char popTyped() {
        if (typed_.empty()) return 0;
        const char c = typed_.front();
        typed_.erase(typed_.begin());
        return c;
    }

    // Normalized movement intent in camera space: x = strafe, y = forward.
    Vec2 wish() const { return wish_; }
    int mouseDx() const { return mouseDx_; }
    int mouseDy() const { return mouseDy_; }

private:
    uint16_t actions_ = 0;
    Vec2 wish_{};
    int mouseDx_ = 0;
    int mouseDy_ = 0;
    std::string typed_;
};
