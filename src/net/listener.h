#ifndef XYCO_NET_LISTENER_H_
#define XYCO_NET_LISTENER_H_

#include <unistd.h>

#include "io/mod.h"
#include "net/socket.h"
#include "runtime/future.h"
#include "runtime/registry.h"

namespace net {
class TcpStream;
class TcpListener;

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

class TcpStream {
  template <typename T>
  using Future = runtime::Future<T>;

  friend struct fmt::formatter<TcpStream>;
  friend class TcpSocket;
  friend class TcpListener;

 public:
  static auto connect(SocketAddr addr) -> Future<io::IoResult<TcpStream>>;

  template <typename Iterator>
  auto read(Iterator begin, Iterator end) -> Future<io::IoResult<uintptr_t>> {
    using CoOutput = io::IoResult<uintptr_t>;

    class Future : public runtime::Future<CoOutput> {
     public:
      Future(Iterator begin, Iterator end, net::TcpStream *self)
          : runtime::Future<CoOutput>(nullptr),
            begin_(begin),
            end_(end),
            self_(self) {}

      auto poll(runtime::Handle<void> self)
          -> runtime::Poll<CoOutput> override {
        auto extra = std::get<runtime::IoExtra>(self_->event_->extra_);
        if (extra.readable()) {
          auto n = ::read(self_->socket_.into_c_fd(), &*begin_,
                          std::distance(begin_, end_));
          if (n != -1) {
            INFO("read {} bytes from {}", n, self_->socket_);
            return runtime::Ready<CoOutput>{CoOutput::ok(n)};
          }
          if (errno != EAGAIN && errno != EWOULDBLOCK) {
            return runtime::Ready<CoOutput>{
                CoOutput::err(io::into_sys_result(-1).unwrap_err())};
          }
        }
        extra.clear_readable();
        self_->event_->future_ = this;
        return runtime::Pending();
      }

     private:
      net::TcpStream *self_;
      Iterator begin_;
      Iterator end_;
    };

    auto result = co_await Future(begin, end, this);
    event_->future_ = nullptr;
    co_return result;
  }

  template <typename Iterator>
  auto write(Iterator begin, Iterator end) -> Future<io::IoResult<uintptr_t>> {
    using CoOutput = io::IoResult<uintptr_t>;

    class Future : public runtime::Future<CoOutput> {
     public:
      Future(Iterator begin, Iterator end, TcpStream *self)
          : runtime::Future<CoOutput>(nullptr),
            begin_(begin),
            end_(end),
            self_(self) {}

      auto poll(runtime::Handle<void> self)
          -> runtime::Poll<CoOutput> override {
        auto extra = std::get<runtime::IoExtra>(self_->event_->extra_);
        if (extra.writeable()) {
          auto n = ::write(self_->socket_.into_c_fd(), &*begin_,
                           std::distance(begin_, end_));
          auto nbytes = io::into_sys_result(n).map(
              [](auto n) -> uintptr_t { return static_cast<uintptr_t>(n); });
          INFO("write {} bytes to {}", n, self_->socket_);
          return runtime::Ready<CoOutput>{nbytes};
        }
        extra.clear_writeable();
        self_->event_->future_ = this;
        return runtime::Pending();
      }

     private:
      TcpStream *self_;
      Iterator begin_;
      Iterator end_;
    };

    auto result = co_await Future(begin, end, this);
    event_->future_ = nullptr;
    co_return result;
  }

  static auto flush() -> Future<io::IoResult<void>>;

  [[nodiscard]] auto shutdown(io::Shutdown shutdown) const
      -> Future<io::IoResult<void>>;

  TcpStream(const TcpStream &tcp_stream) = delete;

  TcpStream(TcpStream &&tcp_stream) noexcept = default;

  auto operator=(const TcpStream &tcp_stream) -> TcpStream & = delete;

  auto operator=(TcpStream &&tcp_stream) noexcept -> TcpStream & = default;

  ~TcpStream();

 private:
  explicit TcpStream(Socket &&socket, runtime::IoExtra::State state);

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