#include "Terminal.h"
#include <csignal>
#include <cstdio>
#include <sys/ioctl.h>
#include <unistd.h>

namespace {
Terminal* g_terminal = nullptr;
}  // namespace

void Terminal::handleSignal(int) {
    if (g_terminal != nullptr) g_terminal->restore();
    _exit(0);
}

Terminal::Terminal() {
    g_terminal = this;
    std::signal(SIGINT, &Terminal::handleSignal);
    std::signal(SIGTERM, &Terminal::handleSignal);

    tcgetattr(STDIN_FILENO, &saved_);
    termios raw = saved_;
    raw.c_lflag &= ~(ECHO | ICANON);  // keep ISIG so Ctrl+C still works
    raw.c_cc[VMIN] = 0;
    raw.c_cc[VTIME] = 0;
    tcsetattr(STDIN_FILENO, TCSANOW, &raw);

    std::setvbuf(stdout, nullptr, _IOFBF, 1 << 16);
    std::fputs("\x1b[?1049h\x1b[?25l", stdout);
    std::fflush(stdout);
}

Terminal::~Terminal() {
    restore();
    g_terminal = nullptr;
}

void Terminal::restore() {
    tcsetattr(STDIN_FILENO, TCSANOW, &saved_);
    std::fputs("\x1b[?25h\x1b[?1049l", stdout);
    std::fflush(stdout);
}

int Terminal::width() const {
    winsize ws{};
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) != 0 || ws.ws_col == 0) return 80;
    return ws.ws_col;
}

int Terminal::height() const {
    winsize ws{};
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) != 0 || ws.ws_row == 0) return 24;
    return ws.ws_row;
}

Key Terminal::readKey() {
    unsigned char c = 0;
    if (::read(STDIN_FILENO, &c, 1) != 1) return Key::None;

    switch (c) {
        case 'w': case 'W': return Key::W;
        case 'a': case 'A': return Key::A;
        case 's': case 'S': return Key::S;
        case 'd': case 'D': return Key::D;
        case 'q': case 'Q': return Key::Q;
        case 'e': case 'E': return Key::E;
        case '\t': return Key::Tab;
        case '[': return Key::LBracket;
        case ']': return Key::RBracket;
        case 0x1b: break;
        default: return Key::None;
    }

    unsigned char seq[2] = {0, 0};
    if (::read(STDIN_FILENO, &seq[0], 1) == 1 && seq[0] == '[' &&
        ::read(STDIN_FILENO, &seq[1], 1) == 1) {
        switch (seq[1]) {
            case 'A': return Key::Up;
            case 'B': return Key::Down;
            case 'C': return Key::Right;
            case 'D': return Key::Left;
        }
    }
    return Key::Escape;
}
