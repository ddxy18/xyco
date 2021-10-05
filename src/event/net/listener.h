#ifndef XYWEBSERVER_EVENT_NET_LISTENER_H_
#define XYWEBSERVER_EVENT_NET_LISTENER_H_

#include "event/io/mod.h"
#include "event/net/socket.h"
#include "event/runtime/future.h"

namespace reactor {
class Poll;
}

namespace net {
class TcpStream;
class TcpListener;

enum class Shutdown { Read, Write, All };

class TcpSocket {
  template <typename T>
  using Future = runtime::Future<T>;

  friend struct fmt::formatter<TcpSocket>;

 public:
  auto bind(SocketAddr addr) -> Future<IoResult<void>>;

  auto connect(SocketAddr addr) -> Future<IoResult<TcpStream>>;

  auto listen(int backlog) -> Future<IoResult<TcpListener>>;

  auto set_reuseaddr(bool reuseaddr) -> IoResult<void>;

  auto set_reuseport(bool reuseport) -> IoResult<void>;

  static auto new_v4() -> IoResult<TcpSocket>;

  static auto new_v6() -> IoResult<TcpSocket>;

 private:
  TcpSocket(int fd);

  Socket socket_;
};

class TcpStream : public ReadTrait, WriteTrait {
  template <typename T>
  using Future = runtime::Future<T>;

  friend struct fmt::formatter<TcpStream>;

 public:
  explicit TcpStream(Socket socket);

  static auto connect(SocketAddr addr) -> Future<IoResult<TcpStream>>;

  auto read(std::vector<char> *buf) -> Future<IoResult<uintptr_t>> override;

  auto write(const std::vector<char> &buf)
      -> Future<IoResult<uintptr_t>> override;

  auto write_all(const std::vector<char> &buf)
      -> Future<IoResult<void>> override;

  auto flush() -> Future<IoResult<void>> override;

  [[nodiscard]] auto shutdown(Shutdown shutdown) const
      -> Future<IoResult<void>>;

 private:
  template <typename I>
  auto write(I begin, I end) -> Future<IoResult<uintptr_t>>;

  Socket socket_;
};

class TcpListener {
  friend class TcpSocket;
  friend struct fmt::formatter<TcpListener>;

  template <typename T>
  using Future = runtime::Future<T>;

 public:
  static auto bind(SocketAddr addr) -> Future<IoResult<TcpListener>>;

  auto accept() -> Future<IoResult<std::pair<TcpStream, SocketAddr>>>;

 private:
  TcpListener(int fd);

  Socket socket_;
};
}  // namespace net

template <>
struct fmt::formatter<net::TcpSocket> : public fmt::formatter<bool> {
  template <typename FormatContext>
  auto format(const net::TcpSocket &tcp_socket, FormatContext &ctx) const
      -> decltype(ctx.out());
};

template <>
struct fmt::formatter<net::TcpStream> : public fmt::formatter<bool> {
  template <typename FormatContext>
  auto format(const net::TcpStream &tcp_stream, FormatContext &ctx) const
      -> decltype(ctx.out());
};

template <>
struct fmt::formatter<net::TcpListener> : public fmt::formatter<bool> {
  template <typename FormatContext>
  auto format(const net::TcpListener &tcp_listener, FormatContext &ctx) const
      -> decltype(ctx.out());
};

#endif  // XYWEBSERVER_EVENT_NET_LISTENER_H_