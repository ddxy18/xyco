#include "socket.h"

#include <unistd.h>

#include "io/utils.h"

auto xyco::net::SocketAddrV4::get_port() const -> uint16_t {
  return inner_.sin_port;
}

auto xyco::net::SocketAddrV6::get_port() const -> uint16_t {
  return inner_.sin6_port;
}

xyco::net::Ipv4Addr::Ipv4Addr(const char* s) : inner_() {
  inet_pton(AF_INET, s, &inner_);
}

xyco::net::Ipv6Addr::Ipv6Addr(const char* s) : inner_() {
  inet_pton(AF_INET6, s, &inner_);
}

auto xyco::net::SocketAddr::new_v4(Ipv4Addr ip, uint16_t port) -> SocketAddr {
  auto addrv4 = SocketAddrV4{};
  addrv4.inner_.sin_port = port;
  addrv4.inner_.sin_addr = ip.inner_;
  addrv4.inner_.sin_family = AF_INET;
  auto addr = SocketAddr{};
  addr.addr_ = std::variant<SocketAddrV4, SocketAddrV6>(addrv4);
  return addr;
}

auto xyco::net::SocketAddr::new_v6(Ipv6Addr ip, uint16_t port) -> SocketAddr {
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

auto xyco::net::SocketAddr::is_v4() -> bool { return addr_.index() == 0; }

auto xyco::net::SocketAddr::into_c_addr() const -> const sockaddr* {
  const void* ptr = nullptr;
  if (addr_.index() == 0) {
    ptr = static_cast<const void*>(&std::get<SocketAddrV4>(addr_).inner_);
  } else {
    ptr = static_cast<const void*>(&std::get<SocketAddrV6>(addr_).inner_);
  }

  return static_cast<const sockaddr*>(ptr);
}

auto xyco::net::Socket::into_c_fd() const -> int { return fd_; }

xyco::net::Socket::Socket(int fd) : fd_(fd) {}

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
    io::into_sys_result(::close(fd_)).unwrap();
  }
}

template <typename FormatContext>
auto fmt::formatter<xyco::net::SocketAddr>::format(
    const xyco::net::SocketAddr& addr, FormatContext& ctx) const
    -> decltype(ctx.out()) {
  const auto* sock_addr = addr.into_c_addr();
  std::string ip(INET_ADDRSTRLEN, 0);
  inet_ntop(sock_addr->sa_family, static_cast<const char*>(sock_addr->sa_data),
            ip.data(), ip.size());
  uint16_t port = 0;
  if (std::holds_alternative<xyco::net::SocketAddrV4>(addr.addr_)) {
    port = std::get<xyco::net::SocketAddrV4>(addr.addr_).get_port();
  } else {
    port = std::get<xyco::net::SocketAddrV6>(addr.addr_).get_port();
  }

  return format_to(ctx.out(), "{}:{}", ip.c_str(), port);
}

template <typename FormatContext>
auto fmt::formatter<xyco::net::Socket>::format(const xyco::net::Socket& socket,
                                               FormatContext& ctx) const
    -> decltype(ctx.out()) {
  return format_to(ctx.out(), "Socket{{fd_={}}}", socket.fd_);
}

template auto fmt::formatter<xyco::net::SocketAddr>::format(
    const xyco::net::SocketAddr& addr,
    fmt::basic_format_context<fmt::appender, char>& ctx) const
    -> decltype(ctx.out());

template auto fmt::formatter<xyco::net::Socket>::format(
    const xyco::net::Socket& socket,
    fmt::basic_format_context<fmt::appender, char>& ctx) const
    -> decltype(ctx.out());