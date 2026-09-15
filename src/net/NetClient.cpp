#include "net/NetClient.h"
#include "net/Protocol.h"
#include "net/Socket.h"
#include <chrono>
#include <cstdio>
#include <thread>

namespace {
// How long connect() may block waiting for the Welcome message.
constexpr auto kWelcomeTimeout = std::chrono::seconds(3);
}  // namespace

NetClient::~NetClient() { disconnect(); }

bool NetClient::connect(const char* host, uint16_t port) {
    fd_ = sock::connectTo(host, port);
    if (fd_ < 0) return false;
    if (!sock::sendAll(fd_, reinterpret_cast<const char*>(
                                 proto::makeHello("player").data()),
                       static_cast<int>(proto::kHelloSize))) {
        disconnect();
        return false;
    }
    // The socket is non-blocking; poll for the Welcome with short sleeps.
    const auto deadline = std::chrono::steady_clock::now() + kWelcomeTimeout;
    while (std::chrono::steady_clock::now() < deadline) {
        poll();
        if (haveSpawn_) return true;
        if (fd_ < 0) break;  // server hung up before answering
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    disconnect();
    return false;
}

void NetClient::disconnect() {
    if (fd_ >= 0) {
        sock::closeFd(fd_);
        fd_ = -1;
    }
    inBuf_.clear();
    others_.clear();
    haveSpawn_ = false;
}

void NetClient::sendState(float x, float y, float angle) {
    if (fd_ < 0) return;
    const std::vector<uint8_t> msg = proto::makeState(x, y, angle);
    if (!sock::sendAll(fd_, reinterpret_cast<const char*>(msg.data()),
                       static_cast<int>(msg.size()))) {
        disconnect();
    }
}

void NetClient::poll() {
    if (fd_ < 0) return;
    char buf[4096];
    while (true) {
        const int n = sock::readSome(fd_, buf, sizeof buf);
        if (n < 0) break;  // nothing more right now
        if (n == 0) {      // closed or broken
            disconnect();
            return;
        }
        inBuf_.insert(inBuf_.end(), buf, buf + n);
        if (n < static_cast<int>(sizeof buf)) break;
    }
    // Peel complete messages off the front of the buffer.
    size_t pos = 0;
    while (inBuf_.size() - pos >= proto::kHeaderSize) {
        uint16_t len = static_cast<uint16_t>(inBuf_[pos + 2] |
                                             (inBuf_[pos + 3] << 8));
        if (inBuf_.size() - pos < proto::kHeaderSize + len) break;
        if (inBuf_[pos] != proto::kVersion) {
            // Unknown protocol version: the stream is unsalvageable.
            disconnect();
            return;
        }
        handleMessage(std::vector<uint8_t>(inBuf_.begin() + pos,
                                           inBuf_.begin() + pos +
                                               proto::kHeaderSize + len));
        pos += proto::kHeaderSize + len;
    }
    inBuf_.erase(inBuf_.begin(), inBuf_.begin() + static_cast<long>(pos));
}

void NetClient::handleMessage(const std::vector<uint8_t>& msg) {
    proto::Reader r(msg.data() + proto::kHeaderSize,
                    msg.size() - proto::kHeaderSize);
    const uint8_t type = msg[1];
    switch (type) {
        case proto::Welcome: {
            uint32_t id = 0;
            if (r.u32(id) && r.f32(spawnX_) && r.f32(spawnY_)) {
                sessionId_ = id;
                haveSpawn_ = true;
            }
            break;
        }
        case proto::World: {
            uint16_t count = 0;
            if (!r.u16(count)) return;
            others_.clear();
            for (uint16_t i = 0; i < count; ++i) {
                RemotePlayer p{};
                if (!r.u32(p.id) || !r.f32(p.x) || !r.f32(p.y) ||
                    !r.f32(p.angle)) {
                    return;  // torn world: wait for the next one
                }
                if (p.id != sessionId_) others_.push_back(p);
            }
            break;
        }
        default: break;  // Bye carries no state we keep
    }
}
