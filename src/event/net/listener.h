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

 public:
  auto bind(SocketAddr addr) -> Future<IoResult<Void>>;

  auto connect(SocketAddr addr) -> Future<IoResult<TcpStream>>;

  auto listen(int backlog) -> Future<IoResult<TcpListener>>;

  auto set_reuseaddr(bool reuseaddr) -> IoResult<Void>;

  auto set_reuseport(bool reuseport) -> IoResult<Void>;

  static auto new_v4() -> IoResult<TcpSocket>;

  static auto new_v6() -> IoResult<TcpSocket>;

 private:
  TcpSocket(int fd);

  Socket socket_;
};

class TcpStream : public ReadTrait, WriteTrait {
  template <typename T>
  using Future = runtime::Future<T>;

 public:
  explicit TcpStream(Socket socket) : socket_(socket) {}

  static auto connect(SocketAddr addr) -> Future<IoResult<TcpStream>>;

  auto read(std::vector<char> *buf) -> Future<IoResult<uintptr_t>> override;

  auto write(const std::vector<char> &buf)
      -> Future<IoResult<uintptr_t>> override;

  auto write_all(const std::vector<char> &buf)
      -> Future<IoResult<Void>> override;

  auto flush() -> Future<IoResult<Void>> override;

  [[nodiscard]] auto shutdown(Shutdown shutdown) const
      -> Future<IoResult<Void>>;

  Socket socket_;

 private:
  template <typename I>
  auto write(I begin, I end) -> Future<IoResult<uintptr_t>>;
};

class TcpListener {
  friend class TcpSocket;

  template <typename T>
  using Future = runtime::Future<T>;

 public:
  static auto bind(SocketAddr addr) -> Future<IoResult<TcpListener>>;

  auto accept() -> Future<IoResult<std::pair<TcpStream, SocketAddr>>>;

 private:
  TcpListener(int fd);

  Socket socket_;
  reactor::Poll *poll_;
};
}  // namespace net

#endif  // XYWEBSERVER_EVENT_NET_LISTENER_H_