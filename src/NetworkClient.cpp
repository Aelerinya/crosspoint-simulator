#include "NetworkClient.h"

#include <netdb.h>
#include <sys/ioctl.h>
#include <cstdlib>
#include <sys/socket.h>
#include <unistd.h>

#include <cerrno>
#include <cstdio>
#include <cstring>

#include "Stream.h"

struct NetworkClient::Impl {
  explicit Impl(int socketFd) : fd(socketFd) {}
  ~Impl() {
    if (fd >= 0)
      ::close(fd);
  }

  int fd = -1;
};

NetworkClient::NetworkClient(int fd) : impl_(std::make_shared<Impl>(fd)) {}

int NetworkClient::connect(const char *host, uint16_t port) {
  if (!host || host[0] == '\0')
    return 0;

  // Lets a simulated reader reach a peer's web server on a local test port.
  const char *peerPort = std::getenv("CROSSPOINT_SIM_PEER_PORT");
  if (port == 80 && peerPort && *peerPort)
    port = static_cast<uint16_t>(std::atoi(peerPort));
  char service[8] = {};
  snprintf(service, sizeof(service), "%u", static_cast<unsigned>(port));

  addrinfo hints{};
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;

  addrinfo *results = nullptr;
  if (getaddrinfo(host, service, &hints, &results) != 0)
    return 0;

  int fd = -1;
  for (addrinfo *addr = results; addr != nullptr; addr = addr->ai_next) {
    fd = ::socket(addr->ai_family, addr->ai_socktype, addr->ai_protocol);
    if (fd < 0)
      continue;
    if (::connect(fd, addr->ai_addr, addr->ai_addrlen) == 0)
      break;
    ::close(fd);
    fd = -1;
  }
  freeaddrinfo(results);

  if (fd < 0)
    return 0;

  stop();
  impl_ = std::make_shared<Impl>(fd);
  return 1;
}

size_t NetworkClient::write(const uint8_t *buf, size_t size) {
  if (!buf || size == 0)
    return 0;
  if (!impl_)
    return 0;
  if (impl_->fd < 0)
    return 0;

  size_t writtenTotal = 0;
#ifdef MSG_NOSIGNAL
  constexpr int sendFlags = MSG_NOSIGNAL;
#else
  constexpr int sendFlags = 0;
#endif
  while (writtenTotal < size) {
    const ssize_t written =
        ::send(impl_->fd, buf + writtenTotal, size - writtenTotal, sendFlags);
    if (written <= 0)
      break;
    writtenTotal += static_cast<size_t>(written);
  }
  return writtenTotal;
}

size_t NetworkClient::write(Stream &stream) {
  uint8_t buffer[4096];
  size_t total = 0;
  while (stream.available() > 0) {
    size_t count = 0;
    while (count < sizeof(buffer) && stream.available() > 0) {
      const int value = stream.read();
      if (value < 0)
        break;
      buffer[count++] = static_cast<uint8_t>(value);
    }
    if (count == 0)
      break;
    const size_t written = write(buffer, count);
    total += written;
    if (written != count)
      break;
  }
  return total;
}

void NetworkClient::stop() {
  if (impl_ && impl_->fd >= 0) {
    ::close(impl_->fd);
    impl_->fd = -1;
  }
}

int NetworkClient::available() {
  if (!impl_ || impl_->fd < 0)
    return 0;
  int pending = 0;
  if (::ioctl(impl_->fd, FIONREAD, &pending) != 0)
    return 0;
  return pending;
}

int NetworkClient::read(uint8_t *buf, size_t size) {
  if (!impl_ || impl_->fd < 0 || !buf || size == 0)
    return -1;
  const ssize_t n = ::recv(impl_->fd, buf, size, MSG_DONTWAIT);
  if (n == 0) {
    stop();
    return -1;
  }
  return n < 0 ? -1 : static_cast<int>(n);
}

int NetworkClient::read() {
  uint8_t c = 0;
  return read(&c, 1) == 1 ? c : -1;
}

// Arduino reports a closed peer as disconnected once its data is drained.
uint8_t NetworkClient::connected() {
  if (!impl_ || impl_->fd < 0)
    return 0;
  char probe;
  const ssize_t n = ::recv(impl_->fd, &probe, 1, MSG_PEEK | MSG_DONTWAIT);
  return n == 0 ? 0 : 1;
}
