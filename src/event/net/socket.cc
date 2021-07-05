#include "socket.h"

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>

#include <variant>

#include "fmt/format.h"

auto Ipv4Addr::New(const char *s) -> Ipv4Addr {
  auto addr = Ipv4Addr{};
  addr.inner_.s_addr = inet_addr(s);
  return addr;
}

constexpr auto Ipv6Addr::New(uint16_t a, uint16_t b, uint16_t c, uint16_t d,
                             uint16_t e, uint16_t f, uint16_t g, uint16_t h)
    -> Ipv6Addr {
  return Ipv6Addr{};
}

auto SocketAddr::new_v4(Ipv4Addr ip, uint16_t port) -> SocketAddr {
  auto addrv4 = SocketAddrV4{};
  addrv4.inner_.sin_port = port;
  addrv4.inner_.sin_addr = ip.inner_;
  addrv4.inner_.sin_family = AF_INET;
  auto addr = SocketAddr{};
  addr.addr_ = std::variant<SocketAddrV4, SocketAddrV6>(addrv4);
  return addr;
}

auto SocketAddr::new_v6(Ipv6Addr ip, uint16_t port, uint32_t flowinfo,
                        uint32_t scope_id) -> SocketAddr {
  auto addrv6 = SocketAddrV6{};
  auto addr = SocketAddr{};
  addr.addr_ = std::variant<SocketAddrV4, SocketAddrV6>(addrv6);
  return addr;
}

auto Socket::New(SocketAddr addr, int type) -> Socket {
  auto sock = Socket{};
  auto family = AF_INET;
  if (addr.addr_.index() == 1) {
    family = AF_INET6;
  }
  sock.fd_ = socket(family, type, 0);
  return sock;
}

auto Socket::new_nonblocking(SocketAddr addr, int type) -> Socket {
  return New(addr, type | SOCK_NONBLOCK);
}

auto Socket::New(int fd) -> Socket {
  auto sock = Socket{};
  sock.fd_ = fd;
  return sock;
}

auto SocketAddr::into_c_addr() -> sockaddr * {
  if (addr_.index() == 0) {
    return reinterpret_cast<sockaddr *>(&std::get<SocketAddrV4>(addr_).inner_);
  }
  return reinterpret_cast<sockaddr *>(&std::get<SocketAddrV6>(addr_).inner_);
}

auto Socket::into_c_fd() const -> int { return fd_; }