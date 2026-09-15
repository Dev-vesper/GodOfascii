#pragma once
#include <cstdint>
#include <vector>

// Wire protocol shared by the client and the server. Every message is a
// four-byte header {version, type, payloadLength} followed by the payload.
// Multi-byte numbers travel little-endian; floats as IEEE 754. The payload
// length makes stream parsing safe: a truncated message is kept until the
// rest arrives. Nothing here knows about sockets.
namespace proto {

constexpr uint8_t kVersion = 1;
constexpr size_t kHeaderSize = 4;

enum Type : uint8_t {
    Hello = 1,   // C->S char name[12]; first message on the wire
    Welcome = 2, // S->C u32 sessionId, f32 spawnX, spawnY
    State = 3,   // C->S f32 x, y, angle; ~20 times a second
    World = 4,   // S->C u16 count, then count x {u32 id, f32 x, y, angle}
    Bye = 5,     // S->C u32 sessionId that just left
};

constexpr size_t kHelloSize = kHeaderSize + 12;
constexpr size_t kWelcomeSize = kHeaderSize + 12;
constexpr size_t kStateSize = kHeaderSize + 12;
constexpr size_t kWorldEntrySize = 16;

// -- append side -----------------------------------------------------------
void writeU16(std::vector<uint8_t>& out, uint16_t v);
void writeU32(std::vector<uint8_t>& out, uint32_t v);
void writeF32(std::vector<uint8_t>& out, float v);
std::vector<uint8_t> makeHello(const char* name);
std::vector<uint8_t> makeWelcome(uint32_t sessionId, float spawnX,
                                  float spawnY);
std::vector<uint8_t> makeState(float x, float y, float angle);
std::vector<uint8_t> makeWorld(const std::vector<uint32_t>& ids,
                               const std::vector<float>& pos);  // x,y,angle
std::vector<uint8_t> makeBye(uint32_t sessionId);

// -- parse side ------------------------------------------------------------
// Simple cursor over a filled buffer; reads fail cleanly at the end.
class Reader {
public:
    Reader(const uint8_t* data, size_t size) : data_(data), size_(size) {}
    bool u8(uint8_t& v);
    bool u16(uint16_t& v);
    bool u32(uint32_t& v);
    bool f32(float& v);
    bool skip(size_t n);
    size_t left() const { return size_ - pos_; }

private:
    const uint8_t* data_;
    size_t size_;
    size_t pos_ = 0;
};

}  // namespace proto
