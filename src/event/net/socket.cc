#include "socket.h"

auto Ipv4Addr::New(const char* s) -> Ipv4Addr {
  auto addr = Ipv4Addr{};
  addr.inner_.s_addr = inet_addr(s);
  return addr;
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

auto SocketAddr::new_v6(Ipv6Addr ip, uint16_t port) -> SocketAddr {
  auto addrv6 = SocketAddrV6{};
  addrv6.inner_.sin6_addr = ip.inner_;
  addrv6.inner_.sin6_port = port;
  addrv6.inner_.sin6_family = AF_INET6;
  addrv6.inner_.sin6_flowinfo = 0;
  addrv6.inner_.sin6_scope_id = 0;
  auto addr = SocketAddr{};
  addr.addr_ = std::variant<SocketAddrV4, SocketAddrV6>(addrv6);
  return addr;
}

auto SocketAddr::is_v4() -> bool { return addr_.index() == 0; }

auto SocketAddr::into_c_addr() const -> const sockaddr* {
  const void* ptr = nullptr;
  if (addr_.index() == 0) {
    ptr = static_cast<const void*>(&std::get<SocketAddrV4>(addr_).inner_);
  } else {
    ptr = static_cast<const void*>(&std::get<SocketAddrV6>(addr_).inner_);
  }

  return static_cast<const sockaddr*>(ptr);
}

Socket::Socket(int fd) : fd_(fd) {}

auto Socket::into_c_fd() const -> int { return fd_; }