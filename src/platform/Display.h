#pragma once
#include <memory>

struct CharGrid;
class Input;

// A presentation backend: renders character frames and captures input into
// the shared Input snapshot. Implementations pick the best mechanism for the
// environment -- an SDL window when a graphical session exists, ANSI escape
// sequences on a plain terminal otherwise.
class Display {
public:
    virtual ~Display() = default;

    virtual bool ok() const = 0;
    virtual int cols() const = 0;  // grid dimensions for the current size
    virtual int rows() const = 0;
    virtual void present(const CharGrid& grid) = 0;
    // Refreshes the input snapshot for this frame. Implementations reset the
    // snapshot first, then fill it with events observed since the last call.
    virtual void pollInput(Input& input) = 0;
    virtual void toggleFullscreen() {}

    // Selects a backend at runtime: the SDL window when a graphical session
    // is available, otherwise the terminal. ASCII3D_BACKEND=terminal or =sdl
    // forces a specific one. Prints the reason to stderr and returns a
    // display with ok() == false when neither works.
    static std::unique_ptr<Display> create();
};
