#ifndef XYCO_NET_LISTENER_H_
#define XYCO_NET_LISTENER_H_

#include "io/mod.h"
#include "net/socket.h"
#include "runtime/future.h"
#include "runtime/registry.h"

namespace net {
class TcpStream;
class TcpListener;

enum class Shutdown { Read, Write, All };

class TcpSocket {
  template <typename T>
  using Future = runtime::Future<T>;

  friend struct fmt::formatter<TcpSocket>;

 public:
  auto bind(SocketAddr addr) -> Future<io::IoResult<void>>;

  auto connect(SocketAddr addr) -> Future<io::IoResult<TcpStream>>;

  auto listen(int backlog) -> Future<io::IoResult<TcpListener>>;

  auto set_reuseaddr(bool reuseaddr) -> io::IoResult<void>;

  auto set_reuseport(bool reuseport) -> io::IoResult<void>;

  static auto new_v4() -> io::IoResult<TcpSocket>;

  static auto new_v6() -> io::IoResult<TcpSocket>;

 private:
  TcpSocket(Socket &&socket);

  Socket socket_;
};

class TcpStream : public io::ReadTrait, io::WriteTrait {
  template <typename T>
  using Future = runtime::Future<T>;

  friend struct fmt::formatter<TcpStream>;
  friend class TcpSocket;
  friend class TcpListener;

 public:
  static auto connect(SocketAddr addr) -> Future<io::IoResult<TcpStream>>;

  auto read(std::vector<char> *buf) -> Future<io::IoResult<uintptr_t>> override;

  auto write(const std::vector<char> &buf)
      -> Future<io::IoResult<uintptr_t>> override;

  auto write_all(const std::vector<char> &buf)
      -> Future<io::IoResult<void>> override;

  auto flush() -> Future<io::IoResult<void>> override;

  [[nodiscard]] auto shutdown(Shutdown shutdown) const
      -> Future<io::IoResult<void>>;

  TcpStream(const TcpStream &tcp_stream) = delete;

  TcpStream(TcpStream &&tcp_stream) noexcept = default;

  auto operator=(const TcpStream &tcp_stream) -> TcpStream & = delete;

  auto operator=(TcpStream &&tcp_stream) noexcept -> TcpStream & = default;

  ~TcpStream();

 private:
  template <typename I>
  auto write(I begin, I end) -> Future<io::IoResult<uintptr_t>>;

  explicit TcpStream(Socket &&socket, runtime::Event::State state);

  Socket socket_;
  std::unique_ptr<runtime::Event> event_;
};

class TcpListener {
  friend class TcpSocket;
  friend struct fmt::formatter<TcpListener>;

  template <typename T>
  using Future = runtime::Future<T>;

 public:
  static auto bind(SocketAddr addr) -> Future<io::IoResult<TcpListener>>;

  auto accept() -> Future<io::IoResult<std::pair<TcpStream, SocketAddr>>>;

  TcpListener(const TcpListener &tcp_stream) = delete;

  TcpListener(TcpListener &&tcp_stream) noexcept = default;

  auto operator=(const TcpListener &tcp_stream) -> TcpListener & = delete;

  auto operator=(TcpListener &&tcp_stream) noexcept -> TcpListener & = default;

  ~TcpListener();

 private:
  TcpListener(Socket &&socket);

  Socket socket_;
  std::unique_ptr<runtime::Event> event_;
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

#endif  // XYCO_NET_LISTENER_H_