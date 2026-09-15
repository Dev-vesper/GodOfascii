#pragma once
#include <cstdint>
#include <string>

// Thin POSIX TCP socket helpers. Negative return means failure. The client
// sockets are non-blocking so the game loop can poll them every frame.
namespace sock {

// Binds and listens on all interfaces; returns the listener fd.
int listenOn(uint16_t port);
// Accepts a pending connection (non-blocking when none), -1 if none/failed.
int acceptClient(int listenFd);
// Connects to host:port; the fd is non-blocking with TCP_NODELAY. -1 on
// failure. Blocking connect (the game calls this once at startup).
int connectTo(const std::string& host, uint16_t port);
// Reads as much as available; returns bytes read, 0 on orderly close,
// -1 when nothing available or an error worth dropping the socket for.
int readSome(int fd, char* buf, int cap);
// Sends everything; false on failure.
bool sendAll(int fd, const char* buf, int len);
void closeFd(int fd);
// Best-guess LAN IPv4 of this machine, "127.0.0.1" when no route exists.
// Sends no packets: the UDP connect only asks the OS for the default
// route's source address.
std::string localAddress();

}  // namespace sock
