#pragma once
#include <memory>

struct CharGrid;

// The platform layer's window: owns the SDL backend, rasterizes character
// cells with the embedded 8x8 font and captures the mouse. All SDL types are
// hidden behind the private implementation, so nothing outside this module
// needs to know about SDL.
class Window {
public:
    Window();
    ~Window();
    Window(const Window&) = delete;
    Window& operator=(const Window&) = delete;

    bool ok() const;
    int cols() const;  // grid dimensions for the current window size
    int rows() const;

    void present(const CharGrid& grid);
    void toggleFullscreen();

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};
