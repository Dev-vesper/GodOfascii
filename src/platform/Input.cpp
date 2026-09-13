#include "platform/Input.h"
#include <SDL.h>

void Input::beginFrame() {
    minimap_ = fullscreen_ = fovNarrow_ = fovWiden_ = false;
    mouseDx_ = mouseDy_ = 0;

    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        switch (event.type) {
            case SDL_QUIT:
                quit_ = true;
                break;
            case SDL_KEYDOWN:
                switch (event.key.keysym.sym) {
                    case SDLK_ESCAPE: quit_ = true; break;
                    case SDLK_TAB: minimap_ = true; break;
                    case SDLK_F11: fullscreen_ = true; break;
                    case SDLK_LEFTBRACKET: fovNarrow_ = true; break;
                    case SDLK_RIGHTBRACKET: fovWiden_ = true; break;
                    default: break;
                }
                break;
            default: break;
        }
    }

    int mx = 0;
    int my = 0;
    SDL_GetRelativeMouseState(&mx, &my);
    mouseDx_ = mx;
    mouseDy_ = my;

    const Uint8* keys = SDL_GetKeyboardState(nullptr);
    Vec2 wish{};
    if (keys[SDL_SCANCODE_W] || keys[SDL_SCANCODE_UP]) wish.y += 1.0f;
    if (keys[SDL_SCANCODE_S] || keys[SDL_SCANCODE_DOWN]) wish.y -= 1.0f;
    if (keys[SDL_SCANCODE_D]) wish.x += 1.0f;
    if (keys[SDL_SCANCODE_A]) wish.x -= 1.0f;
    wish_ = length(wish) > 0.0f ? normalized(wish) : wish;
}

bool Input::triggered(Action action) const {
    switch (action) {
        case Action::Quit: return quit_;
        case Action::ToggleMinimap: return minimap_;
        case Action::ToggleFullscreen: return fullscreen_;
        case Action::FovNarrow: return fovNarrow_;
        case Action::FovWiden: return fovWiden_;
    }
    return false;
}
