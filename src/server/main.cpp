#include "net/Protocol.h"
#include "net/Socket.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <poll.h>
#include <string>
#include <unordered_map>
#include <unistd.h>
#include <vector>

namespace {

// One connected player. The fd owns the session: when it dies the session
// is gone. Positions are whatever the client last reported -- the server
// only relays, it runs no simulation.
struct Session {
    int fd;
    uint32_t id;
    float x = 0.0f;
    float y = 0.0f;
    float angle = 0.0f;
    // Sessions that never reported a position (the host dashboard's
    // observer) stay invisible to players.
    bool hasState = false;
    std::vector<uint8_t> inBuf;
};

// The map spawn point, parsed straight from the map file (same '@' marker
// the game uses) so joining players land where the map intends, with a
// small per-session offset so they do not stack on one cell.
bool mapSpawn(const char* path, float& sx, float& sy) {
    std::ifstream in(path);
    if (!in) return false;
    std::string line;
    int y = 0;
    while (std::getline(in, line)) {
        const size_t x = line.find('@');
        if (x != std::string::npos) {
            sx = static_cast<float>(x) + 0.5f;
            sy = static_cast<float>(y) + 0.5f;
            return true;
        }
        ++y;
    }
    return false;
}

uint16_t portFromArgs(int argc, char** argv) {
    for (int i = 1; i < argc; ++i) {
        const char* p = std::strstr(argv[i], "--port=");
        if (p != nullptr) return static_cast<uint16_t>(std::atoi(p + 7));
    }
    const char* env = std::getenv("ASCII3D_PORT");
    if (env != nullptr && env[0] != '\0') {
        return static_cast<uint16_t>(std::atoi(env));
    }
    return 7777;
}

void dropSession(std::unordered_map<int, Session>& sessions, int fd) {
    sock::closeFd(fd);
    sessions.erase(fd);
}

// Feeds complete messages from one session's buffer to `handler`.
template <typename Handler>
void drain(Session& s, Handler handler) {
    size_t pos = 0;
    while (s.inBuf.size() - pos >= proto::kHeaderSize) {
        const uint16_t len = static_cast<uint16_t>(
            s.inBuf[pos + 2] | (s.inBuf[pos + 3] << 8));
        if (s.inBuf.size() - pos < proto::kHeaderSize + len) break;
        if (s.inBuf[pos] != proto::kVersion) {
            s.inBuf.clear();
            return;  // garbage stream: caller drops the session via read 0
        }
        handler(s, s.inBuf[pos + 1],
                s.inBuf.data() + pos + proto::kHeaderSize, len);
        pos += proto::kHeaderSize + len;
    }
    s.inBuf.erase(s.inBuf.begin(), s.inBuf.begin() + static_cast<long>(pos));
}

}  // namespace

int main(int argc, char** argv) {
    // Join/leave lines must reach the console as they happen, not in one
    // flush when the process dies.
    std::setvbuf(stdout, nullptr, _IOLBF, 0);
    const uint16_t port = portFromArgs(argc, argv);
    const char* mapPath = argc > 1 && std::strncmp(argv[1], "--port=", 7) != 0
                              ? argv[1]
                              : "assets/map.txt";
    float spawnX = 2.5f;
    float spawnY = 2.5f;
    if (!mapSpawn(mapPath, spawnX, spawnY)) {
        std::fprintf(stderr, "warning: no '@' in %s, spawning at 2.5,2.5\n",
                     mapPath);
    }
    const int listener = sock::listenOn(port);
    if (listener < 0) {
        std::fprintf(stderr, "error: cannot listen on port %u\n", port);
        return 1;
    }
    // Optional readiness pipe (--ready-fd=N): one byte once the port is
    // bound, EOF on failure. Lets a parent tell "our server is up" apart
    // from "someone else owns that port".
    for (int i = 1; i < argc; ++i) {
        const char* r = std::strstr(argv[i], "--ready-fd=");
        if (r == nullptr) continue;
        const int fd = std::atoi(r + 11);
        if (fd < 0) break;
        const char byte = 'r';
        ssize_t ignored = write(fd, &byte, 1);
        (void)ignored;
        close(fd);
        break;
    }
    std::printf("ascii3d-server listening on %u (Ctrl+C to stop)\n", port);
    std::fflush(stdout);

    std::unordered_map<int, Session> sessions;
    uint32_t nextId = 1;

    while (true) {
        std::vector<pollfd> fds;
        fds.push_back({listener, POLLIN, 0});
        for (const auto& e : sessions) {
            fds.push_back({e.first, POLLIN, 0});
        }
        // 30ms tick: broadcast the world even when nothing new arrived, so
        // joiners and leavers propagate without waiting for traffic.
        const int ready = poll(fds.data(), static_cast<nfds_t>(fds.size()), 30);
        if (ready < 0) continue;

        for (size_t i = 1; i < fds.size(); ++i) {  // client fds follow [0]
            if ((fds[i].revents & (POLLIN | POLLHUP | POLLERR)) == 0) {
                continue;
            }
            const int fd = fds[i].fd;
            Session& s = sessions[fd];
            char buf[4096];
            const int n = sock::readSome(fd, buf, sizeof buf);
            if (n == 0) {
                std::printf("session %u left\n", s.id);
                dropSession(sessions, fd);
                continue;
            }
            if (n > 0) {
                s.inBuf.insert(s.inBuf.end(), buf, buf + n);
                drain(s, [&](Session& self, uint8_t type, const uint8_t* body,
                             uint16_t len) {
                    if (type != proto::State || len < 12) return;
                    proto::Reader r(body, len);
                    float x, y, a;
                    if (r.f32(x) && r.f32(y) && r.f32(a)) {
                        self.x = x;
                        self.y = y;
                        self.angle = a;
                        self.hasState = true;
                    }
                });
            }
        }

        // Accept joiners (checking the listener last keeps the fds loop
        // above valid -- new sessions simply join the next tick).
        if ((fds[0].revents & POLLIN) != 0) {
            const int fd = sock::acceptClient(listener);
            if (fd >= 0) {
                Session s{};
                s.fd = fd;
                s.id = nextId++;
                // Nudge within the spawn cell (|off| < 0.5 so nobody lands
                // inside a wall) so stacked players stay distinguishable.
                const float dx = (static_cast<float>(s.id % 3) - 1.0f) * 0.3f;
                const float dy = (static_cast<float>((s.id / 3) % 3) - 1.0f) * 0.3f;
                const std::vector<uint8_t> welcome =
                    proto::makeWelcome(s.id, spawnX + dx, spawnY + dy);
                if (sock::sendAll(fd, reinterpret_cast<const char*>(
                                           welcome.data()),
                                  static_cast<int>(welcome.size()))) {
                    sessions.emplace(fd, s);
                    std::printf("session %u joined\n", s.id);
                } else {
                    sock::closeFd(fd);
                }
            }
        }

        // Broadcast the world snapshot to everyone (only positioned
        // sessions appear in it).
        std::vector<uint32_t> ids;
        std::vector<float> pos;
        ids.reserve(sessions.size());
        pos.reserve(sessions.size() * 3);
        for (const auto& e : sessions) {
            if (!e.second.hasState) continue;
            ids.push_back(e.second.id);
            pos.push_back(e.second.x);
            pos.push_back(e.second.y);
            pos.push_back(e.second.angle);
        }
        const std::vector<uint8_t> world = proto::makeWorld(ids, pos);
        for (auto it = sessions.begin(); it != sessions.end();) {
            if (!sock::sendAll(it->first,
                               reinterpret_cast<const char*>(world.data()),
                               static_cast<int>(world.size()))) {
                std::printf("session %u left\n", it->second.id);
                sock::closeFd(it->first);
                it = sessions.erase(it);
            } else {
                ++it;
            }
        }
    }
}
