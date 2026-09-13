#include "platform/Display.h"
#include "platform/SdlDisplay.h"
#include "platform/Terminal.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>

std::unique_ptr<Display> Display::create() {
    const char* forced = std::getenv("ASCII3D_BACKEND");
    if (forced != nullptr) {
        if (std::strcmp(forced, "sdl") == 0) {
            return std::make_unique<SdlDisplay>();
        }
        if (std::strcmp(forced, "terminal") == 0) {
            return std::make_unique<Terminal>(true);
        }
    }

    auto sdl = std::make_unique<SdlDisplay>();
    if (sdl->ok()) return sdl;

    auto term = std::make_unique<Terminal>(false);
    if (term->ok()) return term;

    std::fprintf(stderr,
                 "error: no graphical session and stdout is not a terminal\n");
    return sdl;
}
