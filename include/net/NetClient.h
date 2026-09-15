#pragma once
#include <cstdint>
#include <vector>

// One remote player as the online client sees it. Lives here so the
// rendering pass can share it without depending on NetClient itself.
struct RemotePlayer {
    uint32_t id;
    float x;
    float y;
    float angle;
};

// Client side of the online session: connects to the server, introduces
// itself and keeps a live picture of the other players. The game calls
// sendState() once per frame and poll() once per frame; poll() never
// blocks. When the connection drops the game continues offline.
class NetClient {
public:

    NetClient() = default;
    ~NetClient();
    NetClient(const NetClient&) = delete;
    NetClient& operator=(const NetClient&) = delete;

    // Blocking connect (called once at startup). Sends Hello and waits
    // briefly for the Welcome carrying this player's session id and spawn
    // point. Returns false when the server is unreachable.
    bool connect(const char* host, uint16_t port);
    void disconnect();
    bool connected() const { return fd_ >= 0; }
    uint32_t sessionId() const { return sessionId_; }

    // True between a successful connect() and receiving the Welcome.
    bool hasSpawn() const { return haveSpawn_; }
    float spawnX() const { return spawnX_; }
    float spawnY() const { return spawnY_; }

    void sendState(float x, float y, float angle);
    // Drains the socket, refreshes others() and handles the session spawn.
    void poll();

    const std::vector<RemotePlayer>& others() const { return others_; }

private:
    void handleMessage(const std::vector<uint8_t>& msg);

    int fd_ = -1;
    uint32_t sessionId_ = 0;
    bool haveSpawn_ = false;
    float spawnX_ = 0.0f;
    float spawnY_ = 0.0f;
    std::vector<uint8_t> inBuf_;  // accumulate partial TCP messages
    std::vector<RemotePlayer> others_;
};
