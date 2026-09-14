#pragma once
#include "platform/Display.h"
#include <memory>

// ANSI terminal backend: renders escape sequences directly to the tty and
// reads keyboard plus SGR mouse motion. Output is truecolor when COLORTERM
// says the terminal supports it, otherwise the xterm-256 palette
// (ASCII3D_COLOR=truecolor|256 overrides). Used when no graphical session
// exists (SSH, headless machines, the FreeBSD console) -- no external
// library required.
class Terminal : public Display {
public:
    // allowNonTty skips the is-a-terminal check so the backend can be forced
    // (ASCII3D_BACKEND=terminal) for scripted testing.
    explicit Terminal(bool allowNonTty = false);
    ~Terminal() override;
    Terminal(const Terminal&) = delete;
    Terminal& operator=(const Terminal&) = delete;

    bool ok() const override;
    int cols() const override;
    int rows() const override;
    void present(const CharGrid& grid) override;
    void pollInput(Input& input) override;
    // Bytes written by the most recent present() -- for the debug overlay.
    size_t lastFrameBytes() const override;

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};
