#ifndef XYWEBSERVER_EVENT_NET_SOCKET_H_
#define XYWEBSERVER_EVENT_NET_SOCKET_H_

#include <arpa/inet.h>

#include <variant>

#include "spdlog/fmt/fmt.h"

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
struct fmt::formatter<SocketAddr> : public fmt::formatter<bool> {
  template <typename FormatContext>
  auto format(const SocketAddr& addr, FormatContext& ctx) const
      -> decltype(ctx.out());
};

template <>
struct fmt::formatter<Socket> : public fmt::formatter<bool> {
  template <typename FormatContext>
  auto format(const Socket& socket, FormatContext& ctx) const
      -> decltype(ctx.out());
};

#endif  // XYWEBSERVER_EVENT_NET_SOCKET_H_