#ifndef XYCO_IO_NET_SOCKET_H_
#define XYCO_IO_NET_SOCKET_H_

#include <arpa/inet.h>

#include <variant>

#include "spdlog/fmt/fmt.h"

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
  friend struct fmt::formatter<SocketAddr>;

 public:
  static auto new_v4(Ipv4Addr ip_addr, uint16_t port) -> SocketAddr;

  static auto new_v6(Ipv6Addr ip_addr, uint16_t port) -> SocketAddr;

  auto is_v4() -> bool;

  [[nodiscard]] auto into_c_addr() const -> const sockaddr*;

 private:
  std::variant<SocketAddrV4, SocketAddrV6> addr_;
};

class Socket {
  friend struct fmt::formatter<Socket>;

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
struct fmt::formatter<xyco::net::SocketAddr> : public fmt::formatter<bool> {
  template <typename FormatContext>
  auto format(const xyco::net::SocketAddr& addr, FormatContext& ctx) const
      -> decltype(ctx.out());
};

template <>
struct fmt::formatter<xyco::net::Socket> : public fmt::formatter<bool> {
  template <typename FormatContext>
  auto format(const xyco::net::Socket& socket, FormatContext& ctx) const
      -> decltype(ctx.out());
};

#endif  // XYCO_IO_NET_SOCKET_H_