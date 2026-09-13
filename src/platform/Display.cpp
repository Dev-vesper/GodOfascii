#include "platform/Display.h"
#include "platform/Terminal.h"
#ifdef ASCII3D_SDL
#include "platform/SdlDisplay.h"
#endif
#include <cstdio>
#include <cstdlib>
#include <cstring>

std::unique_ptr<Display> Display::create() {
    const char* forced = std::getenv("ASCII3D_BACKEND");
    const bool wantTerminal =
        forced != nullptr && std::strcmp(forced, "terminal") == 0;

#ifdef ASCII3D_SDL
    if (!wantTerminal) {
        auto sdl = std::make_unique<SdlDisplay>();
        if (sdl->ok()) return sdl;
        if (forced != nullptr) {
            std::fprintf(stderr, "error: cannot create an SDL window\n");
            return sdl;
        }
        // No graphical session: fall through to the terminal backend.
    }
#else
    if (forced != nullptr && std::strcmp(forced, "sdl") == 0) {
        std::fprintf(stderr, "error: built without the SDL backend\n");
    }
#endif

    auto term = std::make_unique<Terminal>(wantTerminal);
    if (term->ok()) return term;
    std::fprintf(stderr,
                 "error: no graphical session and stdout is not a terminal\n");
    return term;
}
