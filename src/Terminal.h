#pragma once
#include <termios.h>

enum class Key {
    None, W, A, S, D, Q, E, Up, Down, Left, Right,
    Tab, Escape, LBracket, RBracket,
};

// Owns the terminal: raw mode, hidden cursor, alternate screen buffer.
// Restores the original state on destruction and on SIGINT/SIGTERM.
class Terminal {
public:
    Terminal();
    ~Terminal();
    Terminal(const Terminal&) = delete;
    Terminal& operator=(const Terminal&) = delete;

    int width() const;
    int height() const;
    Key readKey();  // non-blocking; returns Key::None when nothing is pending

private:
    void restore();
    static void handleSignal(int signal);

    termios saved_{};
};
