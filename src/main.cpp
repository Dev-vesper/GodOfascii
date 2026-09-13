#include "game/Game.h"
#include <iostream>

int main(int argc, char** argv) {
    const std::string mapPath = argc > 1 ? argv[1] : "assets/map.txt";
    Game game(mapPath);
    return game.run();
}
