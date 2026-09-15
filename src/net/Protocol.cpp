#include "net/Protocol.h"
#include <cstring>

namespace proto {

void writeU16(std::vector<uint8_t>& out, uint16_t v) {
    out.push_back(static_cast<uint8_t>(v & 0xff));
    out.push_back(static_cast<uint8_t>(v >> 8));
}

void writeU32(std::vector<uint8_t>& out, uint32_t v) {
    for (int i = 0; i < 4; ++i) {
        out.push_back(static_cast<uint8_t>((v >> (8 * i)) & 0xff));
    }
}

void writeF32(std::vector<uint8_t>& out, float v) {
    uint32_t bits = 0;
    std::memcpy(&bits, &v, sizeof bits);
    writeU32(out, bits);
}

static void header(std::vector<uint8_t>& out, uint8_t type, uint16_t len) {
    out.push_back(kVersion);
    out.push_back(type);
    writeU16(out, len);
}

std::vector<uint8_t> makeHello(const char* name) {
    std::vector<uint8_t> out;
    out.reserve(kHelloSize);
    header(out, Hello, 12);
    for (int i = 0; i < 12; ++i) {
        out.push_back(name != nullptr && name[i] != '\0'
                          ? static_cast<uint8_t>(name[i])
                          : 0);
    }
    return out;
}

std::vector<uint8_t> makeWelcome(uint32_t sessionId, float spawnX,
                                  float spawnY) {
    std::vector<uint8_t> out;
    out.reserve(kWelcomeSize);
    header(out, Welcome, 12);
    writeU32(out, sessionId);
    writeF32(out, spawnX);
    writeF32(out, spawnY);
    return out;
}

std::vector<uint8_t> makeState(float x, float y, float angle) {
    std::vector<uint8_t> out;
    out.reserve(kStateSize);
    header(out, State, 12);
    writeF32(out, x);
    writeF32(out, y);
    writeF32(out, angle);
    return out;
}

std::vector<uint8_t> makeWorld(const std::vector<uint32_t>& ids,
                               const std::vector<float>& pos) {
    // pos holds ids.size() triples: x, y, angle.
    std::vector<uint8_t> out;
    out.reserve(kHeaderSize + 2 + ids.size() * kWorldEntrySize);
    header(out, World,
           static_cast<uint16_t>(2 + ids.size() * kWorldEntrySize));
    writeU16(out, static_cast<uint16_t>(ids.size()));
    for (size_t i = 0; i < ids.size(); ++i) {
        writeU32(out, ids[i]);
        writeF32(out, pos[i * 3]);
        writeF32(out, pos[i * 3 + 1]);
        writeF32(out, pos[i * 3 + 2]);
    }
    return out;
}

std::vector<uint8_t> makeBye(uint32_t sessionId) {
    std::vector<uint8_t> out;
    out.reserve(kHeaderSize + 4);
    header(out, Bye, 4);
    writeU32(out, sessionId);
    return out;
}

bool Reader::u8(uint8_t& v) {
    if (pos_ >= size_) return false;
    v = data_[pos_++];
    return true;
}

bool Reader::u16(uint16_t& v) {
    if (left() < 2) return false;
    v = static_cast<uint16_t>(data_[pos_] | (data_[pos_ + 1] << 8));
    pos_ += 2;
    return true;
}

bool Reader::u32(uint32_t& v) {
    if (left() < 4) return false;
    v = 0;
    for (int i = 0; i < 4; ++i) {
        v |= static_cast<uint32_t>(data_[pos_ + i]) << (8 * i);
    }
    pos_ += 4;
    return true;
}

bool Reader::f32(float& v) {
    uint32_t bits = 0;
    if (!u32(bits)) return false;
    std::memcpy(&v, &bits, sizeof v);
    return true;
}

bool Reader::skip(size_t n) {
    if (left() < n) return false;
    pos_ += n;
    return true;
}

}  // namespace proto
