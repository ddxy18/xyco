#ifndef XYCO_NET_SOCKET_H_
#define XYCO_NET_SOCKET_H_

#include <arpa/inet.h>

#include <format>
#include <variant>

namespace xyco::net {
class SocketAddrV4 {
  friend class SocketAddr;

 public:
  [[nodiscard]] auto get_port() const -> uint16_t;

 private:
  sockaddr_in inner_;
};

class SocketAddrV6 {
  friend class SocketAddr;

 public:
  [[nodiscard]] auto get_port() const -> uint16_t;

 private:
  sockaddr_in6 inner_;
};

class Ipv4Addr {
  friend class SocketAddr;

 public:
  Ipv4Addr(const char* addr = nullptr);

 private:
  in_addr inner_;
};

class Ipv6Addr {
  friend class SocketAddr;

 public:
  Ipv6Addr(const char* addr);

 private:
  in6_addr inner_;
};

class SocketAddr {
  friend struct std::formatter<SocketAddr>;

 public:
  static auto new_v4(Ipv4Addr ip_addr, uint16_t port) -> SocketAddr;

  static auto new_v6(Ipv6Addr ip_addr, uint16_t port) -> SocketAddr;

  auto is_v4() -> bool;

  [[nodiscard]] auto into_c_addr() const -> const sockaddr*;

 private:
  std::variant<SocketAddrV4, SocketAddrV6> addr_;
};

class Socket {
  friend struct std::formatter<Socket>;

 public:
  [[nodiscard]] auto into_c_fd() const -> int;

  Socket(int file_descriptor);

  Socket(const Socket& socket) = delete;

  Socket(Socket&& socket) noexcept;

  auto operator=(const Socket& socket) -> Socket& = delete;

  auto operator=(Socket&& socket) noexcept -> Socket&;

  ~Socket();

 private:
  int fd_;
};
}  // namespace xyco::net

template <>
struct std::formatter<xyco::net::SocketAddr> : public std::formatter<bool> {
  template <typename FormatContext>
  auto format(const xyco::net::SocketAddr& addr, FormatContext& ctx) const
      -> decltype(ctx.out()) {
    const auto* sock_addr = addr.into_c_addr();
    std::string ip_addr(INET6_ADDRSTRLEN, 0);
    uint16_t port = -1;

    if (addr.addr_.index() == 0) {
      ip_addr = inet_ntop(
          sock_addr->sa_family,
          static_cast<const void*>(&static_cast<const sockaddr_in*>(
                                        static_cast<const void*>(sock_addr))
                                        ->sin_addr),
          ip_addr.data(), ip_addr.size());
      port = std::get<0>(addr.addr_).get_port();
    } else {
      ip_addr = inet_ntop(
          sock_addr->sa_family,
          static_cast<const void*>(&static_cast<const sockaddr_in6*>(
                                        static_cast<const void*>(sock_addr))
                                        ->sin6_addr),
          ip_addr.data(), ip_addr.size());
      port = std::get<1>(addr.addr_).get_port();
    }

    return std::format_to(ctx.out(), "SocketAddr{{ip={},port={}}}", ip_addr,
                          port);
  }
};

template <>
struct std::formatter<xyco::net::Socket> : public std::formatter<bool> {
  template <typename FormatContext>
  auto format(const xyco::net::Socket& socket, FormatContext& ctx) const
      -> decltype(ctx.out()) {
    return std::format_to(ctx.out(), "Socket{{fd_={}}}", socket.fd_);
  }
};

#endif  // XYCO_NET_SOCKET_H_
