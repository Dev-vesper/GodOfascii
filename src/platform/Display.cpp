#include "platform/Display.h"
#include "platform/Terminal.h"
#ifdef ASCII3D_SDL
#include "platform/SdlDisplay.h"
#endif
#include <cstdio>
#include <cstdlib>
#include <cstring>

namespace {

// A window needs a graphical session to attach to. Bare consoles (FreeBSD
// vt, Linux VT) have none -- trying SDL there makes its kmsdrm driver grab
// the display and blank the console.
bool graphicalSession() {
    const char* x = std::getenv("DISPLAY");
    const char* wl = std::getenv("WAYLAND_DISPLAY");
    return (x != nullptr && x[0] != '\0') || (wl != nullptr && wl[0] != '\0');
}

}  // namespace

std::unique_ptr<Display> Display::create() {
    const char* forced = std::getenv("ASCII3D_BACKEND");
    const bool wantTerminal =
        forced != nullptr && std::strcmp(forced, "terminal") == 0;
    const bool wantSdl = forced != nullptr && std::strcmp(forced, "sdl") == 0;

#ifdef ASCII3D_SDL
    if (!wantTerminal && (graphicalSession() || wantSdl)) {
        auto sdl = std::make_unique<SdlDisplay>();
        if (sdl->ok()) return sdl;
        if (wantSdl) {
            std::fprintf(stderr, "error: cannot create an SDL window\n");
            return sdl;
        }
        // The session exists but the window failed: fall through to the
        // terminal backend.
    }
#else
    if (wantSdl) {
        std::fprintf(stderr, "error: built without the SDL backend\n");
    }
#endif

    auto term = std::make_unique<Terminal>(wantTerminal);
    if (term->ok()) return term;
    std::fprintf(stderr,
                 "error: no graphical session and stdout is not a terminal\n");
    return term;
}
