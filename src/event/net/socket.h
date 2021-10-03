#ifndef XYWEBSERVER_EVENT_NET_SOCKET_H_
#define XYWEBSERVER_EVENT_NET_SOCKET_H_

#include <arpa/inet.h>

#include <variant>

#include "spdlog/fmt/fmt.h"

class SocketAddrV4 {
  friend class SocketAddr;

 public:
  [[nodiscard]] auto get_port() const -> uint16_t { return inner_.sin_port; }

 private:
  sockaddr_in inner_;
};

class SocketAddrV6 {
  friend class SocketAddr;

 public:
  [[nodiscard]] auto get_port() const -> uint16_t { return inner_.sin6_port; }

 private:
  sockaddr_in6 inner_;
};

class Ipv4Addr {
  friend class SocketAddr;

 public:
  Ipv4Addr(const char* s);

 private:
  in_addr inner_;
};

class Ipv6Addr {
  friend class SocketAddr;

 public:
  Ipv6Addr(const char* s);

 private:
  in6_addr inner_;
};

class SocketAddr {
  friend struct fmt::formatter<SocketAddr>;

 public:
  static auto new_v4(Ipv4Addr ip, uint16_t port) -> SocketAddr;

  static auto new_v6(Ipv6Addr ip, uint16_t port) -> SocketAddr;

  auto is_v4() -> bool;

  [[nodiscard]] auto into_c_addr() const -> const sockaddr*;

 private:
  std::variant<SocketAddrV4, SocketAddrV6> addr_;
};

class Socket {
  friend struct fmt::formatter<Socket>;

 public:
  Socket(int fd);

  [[nodiscard]] auto into_c_fd() const -> int;

 private:
  int fd_;
};

template <>
struct fmt::formatter<SocketAddr> {
  static constexpr auto parse(format_parse_context& ctx)
      -> decltype(ctx.begin()) {
    const auto* it = ctx.begin();
    const auto* end = ctx.end();
    if (it != end && *it != '}') {
      throw format_error("invalid format");
    }

    return it;
  }

  template <typename FormatContext>
  auto format(const SocketAddr& addr, FormatContext& ctx) const
      -> decltype(ctx.out()) {
    const auto* sock_addr = addr.into_c_addr();
    std::string ip(INET_ADDRSTRLEN, 0);
    inet_ntop(sock_addr->sa_family,
              static_cast<const char*>(sock_addr->sa_data), ip.data(),
              ip.size());
    uint16_t port = 0;
    if (std::holds_alternative<SocketAddrV4>(addr.addr_)) {
      port = std::get<SocketAddrV4>(addr.addr_).get_port();
    } else {
      port = std::get<SocketAddrV6>(addr.addr_).get_port();
    }

    return format_to(ctx.out(), "{}:{}", ip, port);
  }
};

template <>
struct fmt::formatter<Socket> {
  static constexpr auto parse(format_parse_context& ctx)
      -> decltype(ctx.begin()) {
    const auto* it = ctx.begin();
    const auto* end = ctx.end();
    if (it != end && *it != '}') {
      throw format_error("invalid format");
    }

    return it;
  }

  template <typename FormatContext>
  auto format(const Socket& socket, FormatContext& ctx) const
      -> decltype(ctx.out()) {
    return format_to(ctx.out(), "Socket{{fd={}}}", socket.fd_);
  }
};

#endif  // XYWEBSERVER_EVENT_NET_SOCKET_H_