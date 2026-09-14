#include "platform/Terminal.h"
#include "platform/Input.h"
#include "render/CharGrid.h"
#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>

#ifdef _WIN32
#include <windows.h>
#include <unordered_set>
#include <vector>
#else
#include <chrono>
#include <unordered_map>
#include <fcntl.h>
#include <signal.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <unistd.h>
#endif

namespace {

// Escape sequences understood by every modern terminal (Windows 10+ too).
constexpr char kSetup[] =
    "\x1b[?1049h"   // alternate screen buffer
    "\x1b[?25l"     // hide cursor
    "\x1b[?7l"      // disable autowrap
    "\x1b[?1003h"   // report all mouse motion
    "\x1b[?1006h";  // ... as SGR sequences
constexpr char kRestore[] =
    "\x1b[?1006l\x1b[?1003l\x1b[?7h\x1b[?25h\x1b[?1049l";

void writeAll(const char* s, size_t n) {
    std::fwrite(s, 1, n, stdout);
    std::fflush(stdout);
}

// Mouse pixels the turn keys inject per frame. Terminals without mouse
// reporting (the FreeBSD vt console, dumb terminals) need a keyboard way
// to look around.
constexpr int kTurnPx = 9;

enum class ColorDepth { True, C256 };

// Truecolor terminals announce themselves via COLORTERM. Anything else --
// the FreeBSD vt console among them -- parses 38;2/48;2 as separate codes
// and renders garbage colors, so those get the xterm-256 palette instead.
ColorDepth colorDepth() {
    const char* forced = std::getenv("ASCII3D_COLOR");
    if (forced != nullptr) {
        return std::strcmp(forced, "truecolor") == 0 ? ColorDepth::True
                                                     : ColorDepth::C256;
    }
    const char* ct = std::getenv("COLORTERM");
    if (ct == nullptr) return ColorDepth::C256;
    return std::strstr(ct, "truecolor") != nullptr ||
                   std::strstr(ct, "24bit") != nullptr
               ? ColorDepth::True
               : ColorDepth::C256;
}

// Nearest xterm-256 palette entry: the 6x6x6 color cube plus a gray ramp.
int quantize256(Rgb c) {
    const int mn = std::min({c.r, c.g, c.b});
    const int mx = std::max({c.r, c.g, c.b});
    if (mx - mn < 16) {  // near-gray: the 24-step ramp is the better fit
        return 232 + std::clamp((c.r + c.g + c.b) / 30, 0, 23);
    }
    const auto cube = [](int v) {
        if (v < 48) return 0;
        if (v < 115) return 1;
        return std::min(5, (v - 35) / 40);
    };
    return 16 + 36 * cube(c.r) + 6 * cube(c.g) + cube(c.b);
}

}  // namespace

#ifdef _WIN32

struct Terminal::Impl {
    bool ok = false;
    bool isConsole = false;
    HANDLE in = INVALID_HANDLE_VALUE;
    HANDLE out = INVALID_HANDLE_VALUE;
    DWORD savedInMode = 0;
    DWORD savedOutMode = 0;
    std::unordered_set<int> held;  // virtual-key codes currently down
    bool haveMouse = false;
    int lastMx = 0;
    int lastMy = 0;

    // Damage tracking for present(): what the terminal currently shows.
    std::vector<Cell> prev;
    int prevW = 0;
    int prevH = 0;
    bool havePrev = false;
    size_t lastBytes = 0;

    void keyEvent(const KEY_EVENT_RECORD& rec, Input& input);
    void buildWish(Input& input);
};

void Terminal::Impl::keyEvent(const KEY_EVENT_RECORD& rec, Input& input) {
    const int vk = static_cast<int>(rec.wVirtualKeyCode);
    const bool down = rec.bKeyDown != FALSE;
    switch (vk) {
        case 'W': case 'A': case 'S': case 'D':
        case 'Q': case 'E':
            if (down) {
                held.insert(vk);
            } else {
                held.erase(vk);
            }
            break;
        case VK_UP:
            if (down) {
                held.insert(vk);
                input.setAction(Action::MenuUp);
            } else {
                held.erase(vk);
            }
            break;
        case VK_DOWN:
            if (down) {
                held.insert(vk);
                input.setAction(Action::MenuDown);
            } else {
                held.erase(vk);
            }
            break;
        case VK_LEFT:
            if (down) input.setAction(Action::MenuLeft);
            break;
        case VK_RIGHT:
            if (down) input.setAction(Action::MenuRight);
            break;
        case VK_RETURN:
            if (down) input.setAction(Action::MenuConfirm);
            break;
        case VK_ESCAPE:
            if (down) input.setAction(Action::MenuToggle);
            break;
        case VK_TAB:
            if (down) input.setAction(Action::ToggleMinimap);
            break;
        case VK_OEM_4:  // '[' on US layouts
            if (down) input.setAction(Action::FovNarrow);
            break;
        case VK_OEM_6:  // ']'
            if (down) input.setAction(Action::FovWiden);
            break;
        case 'C':
            if (down &&
                (rec.dwControlKeyState &
                 (LEFT_CTRL_PRESSED | RIGHT_CTRL_PRESSED)) != 0) {
                input.setAction(Action::Quit);
            }
            break;
        default: break;
    }
}

void Terminal::Impl::buildWish(Input& input) {
    Vec2 wish{};
    if (held.count('W') != 0 || held.count(VK_UP) != 0) wish.y += 1.0f;
    if (held.count('S') != 0 || held.count(VK_DOWN) != 0) wish.y -= 1.0f;
    if (held.count('D') != 0) wish.x += 1.0f;
    if (held.count('A') != 0) wish.x -= 1.0f;
    input.setWish(length(wish) > 0.0f ? normalized(wish) : wish);
}

Terminal::Terminal(bool allowNonTty) : impl_(std::make_unique<Impl>()) {
    impl_->in = GetStdHandle(STD_INPUT_HANDLE);
    impl_->out = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD inMode = 0;
    DWORD outMode = 0;
    impl_->isConsole =
        impl_->in != INVALID_HANDLE_VALUE &&
        impl_->out != INVALID_HANDLE_VALUE &&
        GetConsoleMode(impl_->in, &inMode) &&
        GetConsoleMode(impl_->out, &outMode);
    if (!impl_->isConsole && !allowNonTty) return;

    if (impl_->isConsole) {
        impl_->savedInMode = inMode;
        impl_->savedOutMode = outMode;
        // VT sequences for rendering (available since Windows 10).
        if (!SetConsoleMode(impl_->out,
                            outMode | ENABLE_VIRTUAL_TERMINAL_PROCESSING)) {
            return;
        }
        // Raw-ish input with mouse reporting; quick-edit off so console
        // selection clicks do not freeze the process.
        const DWORD rawIn =
            (inMode & ~(ENABLE_LINE_INPUT | ENABLE_ECHO_INPUT |
                        ENABLE_PROCESSED_INPUT | ENABLE_QUICK_EDIT_MODE)) |
            ENABLE_EXTENDED_FLAGS | ENABLE_MOUSE_INPUT | ENABLE_WINDOW_INPUT;
        SetConsoleMode(impl_->in, rawIn);
    }
    writeAll(kSetup, sizeof(kSetup) - 1);
    impl_->ok = true;
}

Terminal::~Terminal() {
    if (!impl_ || !impl_->ok) return;
    writeAll(kRestore, sizeof(kRestore) - 1);
    if (impl_->isConsole) {
        SetConsoleMode(impl_->in, impl_->savedInMode);
        SetConsoleMode(impl_->out, impl_->savedOutMode);
    }
}

bool Terminal::ok() const { return impl_->ok; }

int Terminal::cols() const {
    CONSOLE_SCREEN_BUFFER_INFO info{};
    if (!GetConsoleScreenBufferInfo(impl_->out, &info)) return 80;
    return info.srWindow.Right - info.srWindow.Left + 1;
}

int Terminal::rows() const {
    CONSOLE_SCREEN_BUFFER_INFO info{};
    if (!GetConsoleScreenBufferInfo(impl_->out, &info)) return 24;
    return info.srWindow.Bottom - info.srWindow.Top + 1;
}

void Terminal::pollInput(Input& input) {
    input.reset();
    DWORD count = 0;
    if (impl_->in != INVALID_HANDLE_VALUE &&
        GetNumberOfConsoleInputEvents(impl_->in, &count) && count > 0) {
        std::vector<INPUT_RECORD> records(count);
        DWORD read = 0;
        if (ReadConsoleInputW(impl_->in, records.data(), count, &read)) {
            for (DWORD i = 0; i < read; ++i) {
                const INPUT_RECORD& r = records[i];
                if (r.EventType == KEY_EVENT) {
                    impl_->keyEvent(r.Event.KeyEvent, input);
                } else if (r.EventType == MOUSE_EVENT) {
                    const MOUSE_EVENT_RECORD& m = r.Event.MouseEvent;
                    if ((m.dwEventFlags & MOUSE_MOVED) != 0) {
                        if (impl_->haveMouse) {
                            input.addMouse(m.dwMousePosition.X - impl_->lastMx,
                                           m.dwMousePosition.Y - impl_->lastMy);
                        }
                        impl_->lastMx = m.dwMousePosition.X;
                        impl_->lastMy = m.dwMousePosition.Y;
                        impl_->haveMouse = true;
                    }
                }
            }
        }
    }
    impl_->buildWish(input);
    // Turn keys: a keyboard way to look around where mouse reporting is
    // unavailable.
    if (impl_->held.count('Q') != 0) input.addMouse(-kTurnPx, 0);
    if (impl_->held.count('E') != 0) input.addMouse(kTurnPx, 0);
}

#else  // POSIX: Linux, FreeBSD and friends

namespace {

using Clock = std::chrono::steady_clock;

// Terminals report no key release; auto-repeat keeps a held key fresh and
// the weight decays until the next repeat (or fades out after release).
constexpr auto kKeyFadeMs = std::chrono::milliseconds(350);
// Console typematic goes completely silent while two keys are held -- no
// repeats for anyone. A chorded key keeps moving for this long without any
// event at all; a single tap on any chord key renews the whole combo.
constexpr auto kCarryMs = std::chrono::milliseconds(2500);

// The key that means the opposite direction, 0 when not a direction key.
// Pressing it brakes a chord partner that went typematic-silent.
char oppositeOf(char c) {
    switch (c) {
        case 'w': return 's';
        case 's': return 'w';
        case 'a': return 'd';
        case 'd': return 'a';
        case 'q': return 'e';
        case 'e': return 'q';
        case '^': return 'v';
        case 'v': return '^';
        default: return 0;
    }
}

termios g_savedTty{};
bool g_isTty = false;

extern "C" void restoreAndExit(int) {
    if (g_isTty) tcsetattr(STDIN_FILENO, TCSANOW, &g_savedTty);
    ssize_t ignored = write(STDOUT_FILENO, kRestore, sizeof(kRestore) - 1);
    (void)ignored;
    _exit(1);
}

}  // namespace

struct Terminal::Impl {
    bool ok = false;
    bool isTty = false;
    // Key (or arrow sentinel '^'/'v') -> last time it was seen pressed.
    std::unordered_map<char, Clock::time_point> held;
    // Same keys -> ride-along clocks that outlive typematic silence while a
    // chord is physically held (see refreshHeld).
    std::unordered_map<char, Clock::time_point> carried;
    bool haveMouse = false;
    int lastMx = 0;
    int lastMy = 0;

    // Damage tracking for present(): what the terminal currently shows.
    std::vector<Cell> prev;
    int prevW = 0;
    int prevH = 0;
    bool havePrev = false;
    size_t lastBytes = 0;

    void key(char ch, Input& input);
    void sequence(const char* params, size_t n, char finalByte, Input& input);
    void applyHeld(Input& input);
    void refreshHeld(bool renewing);
    void chordPress(char ch);
};

// Terminals send nothing while two keys are held -- no repeats for either
// key -- so the newest event for a chord member can be seconds old while it
// is still physically held. Self events drive the fast clock in `held`; the
// ride-along clocks in `carried` keep the whole chord moving through the
// silence. A self-fresh key always carries itself; a stale partner is
// re-carried only when a chord member is pressed again (renewing), so a
// foreign key press neither breaks nor extends the chord.
void Terminal::Impl::refreshHeld(bool renewing) {
    if (held.size() < 2) return;
    const auto now = Clock::now();
    for (const auto& entry : held) {
        const bool selfFresh = now - entry.second < kKeyFadeMs;
        const auto c = carried.find(entry.first);
        const bool carriedFresh =
            c != carried.end() && now - c->second < kCarryMs;
        if (selfFresh || (renewing && carriedFresh)) carried[entry.first] = now;
    }
}

// Shared press handling for keys that feed the movement chord (wasd, qe and
// the arrow sentinels). A re-press of an already-held key renews the whole
// combo; pressing the opposite direction evicts a silent partner as a brake.
void Terminal::Impl::chordPress(char ch) {
    const bool renewing = held.find(ch) != held.end();
    const char opp = oppositeOf(ch);
    const auto it = held.find(opp);
    if (it != held.end() && Clock::now() - it->second >= kKeyFadeMs) {
        held.erase(it);
        carried.erase(opp);
    }
    held[ch] = Clock::now();
    refreshHeld(renewing);
}

void Terminal::Impl::key(char ch, Input& input) {
    if (ch >= 'A' && ch <= 'Z') ch = static_cast<char>(ch - 'A' + 'a');
    switch (ch) {
        case 'w': case 's': case 'a': case 'd': case 'q': case 'e':
            chordPress(ch);
            break;
        case '\t': input.setAction(Action::ToggleMinimap); break;
        case '[': input.setAction(Action::FovNarrow); break;
        case ']': input.setAction(Action::FovWiden); break;
        case '\r': case '\n': input.setAction(Action::MenuConfirm); break;
        case '\x03': input.setAction(Action::Quit); break;  // Ctrl+C
        default: break;
    }
}

void Terminal::Impl::sequence(const char* params, size_t n, char finalByte,
                              Input& input) {
    if (finalByte == 'A') {
        chordPress('^');  // arrow up
        input.setAction(Action::MenuUp);
        return;
    }
    if (finalByte == 'B') {
        chordPress('v');  // arrow down
        input.setAction(Action::MenuDown);
        return;
    }
    if (finalByte == 'C') {  // arrow right
        input.setAction(Action::MenuRight);
        return;
    }
    if (finalByte == 'D') {  // arrow left
        input.setAction(Action::MenuLeft);
        return;
    }
    if (finalByte != 'M' && finalByte != 'm') return;
    // SGR mouse report: params = "<" button ";" x ";" y (1-based cells).
    if (n == 0 || params[0] != '<') return;
    int b = 0;
    int x = 0;
    int y = 0;
    int* dst = &b;
    for (size_t i = 1; i < n; ++i) {
        const char c = params[i];
        if (c == ';') {
            if (dst == &b) {
                dst = &x;
            } else {
                dst = &y;
            }
        } else if (c >= '0' && c <= '9') {
            *dst = *dst * 10 + (c - '0');
        } else {
            return;
        }
    }
    if ((b & 32) == 0) return;  // only motion events turn the camera
    if (haveMouse) input.addMouse(x - lastMx, y - lastMy);
    lastMx = x;
    lastMy = y;
    haveMouse = true;
}

void Terminal::Impl::applyHeld(Input& input) {
    const auto now = Clock::now();
    Vec2 wish{};
    float turn = 0.0f;  // synthetic mouse pixels from the turn keys
    for (auto it = held.begin(); it != held.end();) {
        const auto selfAge = std::chrono::duration_cast<std::chrono::milliseconds>(
            now - it->second);
        // Fast clock: decays between typematic repeats, dies after release.
        float w = selfAge < kKeyFadeMs
                      ? 1.0f - static_cast<float>(selfAge.count()) /
                                   static_cast<float>(kKeyFadeMs.count())
                      : 0.0f;
        const auto c = carried.find(it->first);
        if (c != carried.end()) {
            const auto carryAge =
                std::chrono::duration_cast<std::chrono::milliseconds>(now -
                                                                      c->second);
            if (carryAge < kCarryMs) {
                // Slow clock: full weight for the first half of the window
                // so re-taps do not pulse the Q/E turn rate, then a linear
                // fade so a going-stale chord eases off instead of stopping
                // dead at the deadline.
                const float carryW =
                    std::min(1.0f,
                             2.0f * (1.0f -
                                     static_cast<float>(carryAge.count()) /
                                         static_cast<float>(
                                             kCarryMs.count())));
                w = std::max(w, carryW);
            } else {
                carried.erase(c);
            }
        }
        if (w <= 0.0f) {
            it = held.erase(it);
            continue;
        }
        switch (it->first) {
            case 'w': case '^': wish.y += w; break;
            case 's': case 'v': wish.y -= w; break;
            case 'd': wish.x += w; break;
            case 'a': wish.x -= w; break;
            case 'q': turn -= w; break;
            case 'e': turn += w; break;
            default: break;
        }
        ++it;
    }
    if (length(wish) > 1e-3f) input.setWish(normalized(wish));
    if (turn != 0.0f) input.addMouse(static_cast<int>(turn * kTurnPx), 0);
}

Terminal::Terminal(bool allowNonTty) : impl_(std::make_unique<Impl>()) {
    impl_->isTty = isatty(STDIN_FILENO) && isatty(STDOUT_FILENO);
    if (!impl_->isTty && !allowNonTty) return;
    if (impl_->isTty) {
        tcgetattr(STDIN_FILENO, &g_savedTty);
        termios raw = g_savedTty;
        raw.c_lflag &= ~(ECHO | ICANON | ISIG);
        raw.c_cc[VMIN] = 0;
        raw.c_cc[VTIME] = 0;
        tcsetattr(STDIN_FILENO, TCSANOW, &raw);
        g_isTty = true;
    } else {
        // Forced mode on a pipe: reads must never block.
        const int flags = fcntl(STDIN_FILENO, F_GETFL, 0);
        fcntl(STDIN_FILENO, F_SETFL, flags | O_NONBLOCK);
    }
    signal(SIGINT, restoreAndExit);
    signal(SIGTERM, restoreAndExit);
    signal(SIGHUP, restoreAndExit);
    writeAll(kSetup, sizeof(kSetup) - 1);
    impl_->ok = true;
}

Terminal::~Terminal() {
    if (!impl_ || !impl_->ok) return;
    writeAll(kRestore, sizeof(kRestore) - 1);
    if (g_isTty) tcsetattr(STDIN_FILENO, TCSANOW, &g_savedTty);
    g_isTty = false;
}

bool Terminal::ok() const { return impl_->ok; }

int Terminal::cols() const {
    winsize ws{};
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) != 0 || ws.ws_col == 0) return 80;
    return ws.ws_col;
}

int Terminal::rows() const {
    winsize ws{};
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) != 0 || ws.ws_row == 0) return 24;
    return ws.ws_row;
}

void Terminal::pollInput(Input& input) {
    input.reset();
    char buf[512];
    const ssize_t n = read(STDIN_FILENO, buf, sizeof buf);
    if (n == 0 && !impl_->isTty) {  // stdin closed (piped test run ended)
        input.setAction(Action::Quit);
        return;
    }
    if (n > 0) {
        const size_t len = static_cast<size_t>(n);
        size_t i = 0;
        while (i < len) {
            const char c = buf[i];
            if (c != '\x1b') {
                impl_->key(c, input);
                ++i;
                continue;
            }
            if (i + 1 >= len) {
                input.setAction(Action::MenuToggle);  // lone escape byte
                ++i;
                continue;
            }
            if (buf[i + 1] != '[' && buf[i + 1] != 'O') {
                impl_->key(buf[i + 1], input);  // Alt-modified key
                i += 2;
                continue;
            }
            // CSI/SS3 sequence: scan for the final byte (0x40..0x7e).
            size_t j = i + 2;
            while (j < len && (buf[j] < 0x40 || buf[j] > 0x7e)) ++j;
            if (j >= len) break;  // incomplete, wait for the next read
            impl_->sequence(buf + i + 2, j - (i + 2), buf[j], input);
            i = j + 1;
        }
    }
    impl_->applyHeld(input);
}

#endif  // _WIN32

void Terminal::present(const CharGrid& grid) {
    const int w = grid.width();
    const int h = grid.height();
    const bool full = !impl_->havePrev || impl_->prevW != w ||
                      impl_->prevH != h;
    if (full) impl_->prev.assign(static_cast<size_t>(w) * h, Cell{});
    std::string out;
    out.reserve(static_cast<size_t>(w) * h * 6 + 32);
    // Synchronized update: supporting terminals present the frame atomically
    // instead of tearing through it. Unknown sequences are ignored elsewhere.
    out += "\x1b[?2026h";

    const ColorDepth depth = colorDepth();
    bool haveFg = false;
    bool haveBg = false;
    Rgb curFg{};
    Rgb curBg{};
    int curFgIdx = -1;
    int curBgIdx = -1;
    // Appends one cell, emitting only the SGR codes the pen does not
    // already carry. The pen state survives cursor moves.
    const auto emitCell = [&](const Cell& c) {
        if (depth == ColorDepth::C256) {
            const int bg = quantize256(c.bg);
            if (bg != curBgIdx) {
                out += "\x1b[48;5;";
                out += std::to_string(bg);
                out += 'm';
                curBgIdx = bg;
            }
            if (c.ch != ' ') {
                const int fg = quantize256(c.fg);
                if (fg != curFgIdx) {
                    out += "\x1b[38;5;";
                    out += std::to_string(fg);
                    out += 'm';
                    curFgIdx = fg;
                }
            }
            out += c.ch;
            return;
        }
        if (!haveBg || c.bg != curBg) {
            out += "\x1b[48;2;";
            out += std::to_string(c.bg.r);
            out += ';';
            out += std::to_string(c.bg.g);
            out += ';';
            out += std::to_string(c.bg.b);
            out += 'm';
            curBg = c.bg;
            haveBg = true;
        }
        if (c.ch != ' ' && (!haveFg || c.fg != curFg)) {
            out += "\x1b[38;2;";
            out += std::to_string(c.fg.r);
            out += ';';
            out += std::to_string(c.fg.g);
            out += ';';
            out += std::to_string(c.fg.b);
            out += 'm';
            curFg = c.fg;
            haveFg = true;
        }
        out += c.ch;
    };

    for (int y = 0; y < h; ++y) {
        if (full) {
            // Absolute row positioning: autowrap is disabled, so the cursor
            // never moves to the next line on its own.
            out += "\x1b[";
            out += std::to_string(y + 1);
            out += ";1H";
            out += "\x1b[K";  // wipe what a wider previous frame left behind
            for (int x = 0; x < w; ++x) {
                emitCell(grid.at(x, y));
                impl_->prev[y * w + x] = grid.at(x, y);
            }
            continue;
        }

        // Damaged-cell path: emit runs of changed cells only. A single
        // unchanged cell between two changed ones is re-emitted (cheaper
        // than a new cursor move); a gap of two or more closes the run.
        const Cell* row = &grid.at(0, y);
        Cell* prevRow = &impl_->prev[y * w];
        int x = 0;
        while (x < w) {
            if (row[x] == prevRow[x]) {
                ++x;
                continue;
            }
            int end = x;
            while (end < w) {
                if (row[end] != prevRow[end]) {
                    ++end;
                    continue;
                }
                if (end + 1 < w && row[end + 1] != prevRow[end + 1]) {
                    end += 2;  // bridge a one-cell gap by re-emitting it
                    continue;
                }
                break;
            }
            out += "\x1b[";
            out += std::to_string(y + 1);
            out += ';';
            out += std::to_string(x + 1);
            out += 'H';
            for (int i = x; i < end; ++i) {
                emitCell(row[i]);
                prevRow[i] = row[i];
            }
            x = end;
        }
    }
    out += "\x1b[0m\x1b[?2026l";
    writeAll(out.data(), out.size());
    impl_->prevW = w;
    impl_->prevH = h;
    impl_->havePrev = true;
    impl_->lastBytes = out.size();
}

size_t Terminal::lastFrameBytes() const { return impl_->lastBytes; }
