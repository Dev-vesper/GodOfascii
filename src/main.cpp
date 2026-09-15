#include "game/Game.h"
#include <cstdio>
#include <cstring>
#include <string>

int main(int argc, char** argv) {
    std::string mapPath = "assets/map.txt";
    std::string host;
    uint16_t port = 7777;
    for (int i = 1; i < argc; ++i) {
        if (std::strcmp(argv[i], "--connect") == 0 && i + 1 < argc) {
            const std::string target = argv[++i];
            const size_t colon = target.rfind(':');
            if (colon != std::string::npos) {
                host = target.substr(0, colon);
                port = static_cast<uint16_t>(std::atoi(target.c_str() + colon + 1));
            } else {
                host = target;
            }
        } else {
            mapPath = argv[i];
        }
    }

    Game game(mapPath);
    if (!host.empty() && !game.connect(host, port)) {
        std::fprintf(stderr, "warning: cannot reach %s:%u, playing offline\n",
                     host.c_str(), port);
    }
    return game.run();
}
