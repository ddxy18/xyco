module;

#include <variant>

module xyco.net.common;

import xyco.error;
import xyco.libc;

auto xyco::net::SocketAddrV4::get_port() const -> uint16_t {
  return xyco::libc::ntohs(inner_.sin_port);
}

auto xyco::net::SocketAddrV6::get_port() const -> uint16_t {
  return inner_.sin6_port;
}

xyco::net::Ipv4Addr::Ipv4Addr(const char* addr) : inner_() {
  if (addr == nullptr) {
    inner_.s_addr = xyco::libc::htonl(xyco::libc::K_INADDR_ANY);
  } else {
    xyco::libc::inet_pton(xyco::libc::K_AF_INET, addr, &inner_);
  }
}

xyco::net::Ipv6Addr::Ipv6Addr(const char* addr) : inner_() {
  xyco::libc::inet_pton(xyco::libc::K_AF_INET6, addr, &inner_);
}

auto xyco::net::SocketAddr::new_v4(Ipv4Addr ip_addr,
                                   uint16_t port) -> SocketAddr {
  auto addrv4 = SocketAddrV4{};
  addrv4.inner_.sin_port = xyco::libc::htons(port);
  addrv4.inner_.sin_addr = ip_addr.inner_;
  addrv4.inner_.sin_family = xyco::libc::K_AF_INET;
  auto addr = SocketAddr{};
  addr.addr_ = std::variant<SocketAddrV4, SocketAddrV6>(addrv4);
  return addr;
}

auto xyco::net::SocketAddr::new_v6(Ipv6Addr ip_addr,
                                   uint16_t port) -> SocketAddr {
  auto addrv6 = SocketAddrV6{};
  addrv6.inner_.sin6_addr = ip_addr.inner_;
  addrv6.inner_.sin6_port = port;
  addrv6.inner_.sin6_family = xyco::libc::K_AF_INET6;
  addrv6.inner_.sin6_flowinfo = 0;
  addrv6.inner_.sin6_scope_id = 0;
  auto addr = SocketAddr{};
  addr.addr_ = std::variant<SocketAddrV4, SocketAddrV6>(addrv6);
  return addr;
}

auto xyco::net::SocketAddr::is_v4() -> bool { return addr_.index() == 0; }

auto xyco::net::SocketAddr::into_c_addr() const -> const xyco::libc::sockaddr* {
  const void* ptr = nullptr;
  if (addr_.index() == 0) {
    ptr = static_cast<const void*>(&std::get<SocketAddrV4>(addr_).inner_);
  } else {
    ptr = static_cast<const void*>(&std::get<SocketAddrV6>(addr_).inner_);
  }

  return static_cast<const xyco::libc::sockaddr*>(ptr);
}

auto xyco::net::Socket::into_c_fd() const -> int { return fd_; }

xyco::net::Socket::Socket(int file_descriptor) : fd_(file_descriptor) {}

xyco::net::Socket::Socket(Socket&& socket) noexcept : fd_(socket.fd_) {
  socket.fd_ = -1;
}

auto xyco::net::Socket::operator=(Socket&& socket) noexcept -> Socket& {
  fd_ = socket.fd_;
  socket.fd_ = -1;
  return *this;
}

xyco::net::Socket::~Socket() {
  if (fd_ != -1) {
    *utils::into_sys_result(xyco::libc::close(fd_));
  }
}
