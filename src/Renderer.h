#pragma once
#include <SDL.h>

struct CharGrid;

// Owns the SDL window and draws a CharGrid into it using the embedded 8x8
// font scaled up to 16x16 pixel cells. Also captures the mouse so the game
// receives relative motion for mouselook.
class Renderer {
public:
    Renderer();
    ~Renderer();
    Renderer(const Renderer&) = delete;
    Renderer& operator=(const Renderer&) = delete;

    bool ok() const { return ok_; }
    int cols() const;  // grid dimensions for the current window size
    int rows() const;

    void present(const CharGrid& grid);
    void toggleFullscreen();

private:
    bool buildAtlas();

    SDL_Window* window_ = nullptr;
    SDL_Renderer* renderer_ = nullptr;
    SDL_Texture* atlas_ = nullptr;
    bool ok_ = false;
};
