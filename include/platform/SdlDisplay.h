#pragma once
#include "platform/Display.h"
#include <memory>

// Window backend built on SDL: rasterizes character cells with the embedded
// 8x8 font, captures the mouse with pointer lock and reads the keyboard.
// All SDL types are hidden behind the private implementation, so nothing
// outside this module needs to know about SDL.
class SdlDisplay : public Display {
public:
    SdlDisplay();
    ~SdlDisplay() override;
    SdlDisplay(const SdlDisplay&) = delete;
    SdlDisplay& operator=(const SdlDisplay&) = delete;

    bool ok() const override;
    int cols() const override;
    int rows() const override;
    void present(const CharGrid& grid) override;
    void pollInput(Input& input) override;
    void toggleFullscreen() override;
    bool fullscreen() const override;

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};
