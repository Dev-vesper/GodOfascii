#include "net/Socket.h"
#include <arpa/inet.h>
#include <cerrno>
#include <cstring>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <sys/socket.h>
#include <unistd.h>

namespace sock {

int listenOn(uint16_t port) {
    const int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) return -1;
    const int one = 1;
    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &one, sizeof one);
    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(port);
    if (bind(fd, reinterpret_cast<sockaddr*>(&addr), sizeof addr) != 0 ||
        listen(fd, 16) != 0) {
        close(fd);
        return -1;
    }
    return fd;
}

int acceptClient(int listenFd) {
    const int fd = accept(listenFd, nullptr, nullptr);
    if (fd < 0) return -1;
    const int one = 1;
    setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, &one, sizeof one);
    return fd;
}

int connectTo(const std::string& host, uint16_t port) {
    addrinfo hints{};
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    addrinfo* res = nullptr;
    if (getaddrinfo(host.c_str(), nullptr, &hints, &res) != 0 ||
        res == nullptr) {
        return -1;
    }
    sockaddr_in addr{};
    std::memcpy(&addr, res->ai_addr, sizeof addr);
    freeaddrinfo(res);
    addr.sin_port = htons(port);

    const int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) return -1;
    if (connect(fd, reinterpret_cast<sockaddr*>(&addr), sizeof addr) != 0) {
        close(fd);
        return -1;
    }
    const int one = 1;
    setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, &one, sizeof one);
    // From here on the game only polls: reads must never block a frame.
    const int flags = fcntl(fd, F_GETFL, 0);
    fcntl(fd, F_SETFL, flags | O_NONBLOCK);
    return fd;
}

int readSome(int fd, char* buf, int cap) {
    const ssize_t n = read(fd, buf, static_cast<size_t>(cap));
    if (n > 0) return static_cast<int>(n);
    if (n == 0) return 0;  // orderly close
    if (errno == EAGAIN || errno == EWOULDBLOCK) return -1;
    return 0;  // hard error: treat as a dead connection
}

bool sendAll(int fd, const char* buf, int len) {
    size_t sent = 0;
    while (sent < static_cast<size_t>(len)) {
        const ssize_t n = write(fd, buf + sent, static_cast<size_t>(len) - sent);
        if (n > 0) {
            sent += static_cast<size_t>(n);
            continue;
        }
        if (n < 0 && (errno == EAGAIN || errno == EWOULDBLOCK)) continue;
        if (n < 0 && errno == EINTR) continue;
        return false;
    }
    return true;
}

void closeFd(int fd) { close(fd); }

}  // namespace sock
