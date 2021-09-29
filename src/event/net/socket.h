#ifndef XYWEBSERVER_EVENT_NET_SOCKET_H_
#define XYWEBSERVER_EVENT_NET_SOCKET_H_

#include <netinet/in.h>

#include <variant>

class SocketAddrV4 {
  friend class SocketAddr;

 private:
  sockaddr_in inner_;
};

class SocketAddrV6 {
  friend class SocketAddr;

 private:
  sockaddr_in6 inner_;
};

class Ipv4Addr {
  friend class SocketAddr;

 public:
  static auto New(const char *s) -> Ipv4Addr;

 private:
  in_addr inner_;
};

class Ipv6Addr {
  friend class SocketAddr;

 private:
  in6_addr inner_;
};

class SocketAddr {
  friend class Socket;

 public:
  static auto new_v4(Ipv4Addr ip, uint16_t port) -> SocketAddr;

  static auto new_v6(Ipv6Addr ip, uint16_t port) -> SocketAddr;

  auto into_c_addr() -> sockaddr *;

 private:
  std::variant<SocketAddrV4, SocketAddrV6> addr_;
};

class Socket {
 public:
  static auto New(SocketAddr addr, int type) -> Socket;
  static auto new_nonblocking(SocketAddr addr, int type) -> Socket;
  static auto New(int fd) -> Socket;

  [[nodiscard]] auto into_c_fd() const -> int;

 private:
  int fd_;
};

#endif  // XYWEBSERVER_EVENT_NET_SOCKET_H_