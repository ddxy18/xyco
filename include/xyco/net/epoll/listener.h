#ifndef XYCO_NET_EPOLL_LISTENER_H_
#define XYCO_NET_EPOLL_LISTENER_H_

#include <unistd.h>

#include "xyco/io/epoll/extra.h"
#include "xyco/io/epoll/registry.h"
#include "xyco/io/utils.h"
#include "xyco/net/socket.h"
#include "xyco/runtime/future.h"
#include "xyco/runtime/runtime_ctx.h"
#include "xyco/utils/error.h"
#include "xyco/utils/logger.h"

namespace xyco::net::epoll {
class TcpStream;
class TcpListener;

class TcpSocket {
  template <typename T>
  using Future = runtime::Future<T>;

  friend struct std::formatter<TcpSocket>;

 public:
  auto bind(SocketAddr addr) -> Future<utils::Result<void>>;

  auto connect(SocketAddr addr) -> Future<utils::Result<TcpStream>>;

  auto listen(int backlog) -> Future<utils::Result<TcpListener>>;

  auto set_reuseaddr(bool reuseaddr) -> utils::Result<void>;

  auto set_reuseport(bool reuseport) -> utils::Result<void>;

  static auto new_v4() -> utils::Result<TcpSocket>;

  static auto new_v6() -> utils::Result<TcpSocket>;

 private:
  TcpSocket(Socket &&socket);

  Socket socket_;
};

class TcpStream {
  template <typename T>
  using Future = runtime::Future<T>;

  friend struct std::formatter<TcpStream>;
  friend class TcpSocket;
  friend class TcpListener;

 public:
  static auto connect(SocketAddr addr) -> Future<utils::Result<TcpStream>>;

  template <typename Iterator>
  auto read(Iterator begin, Iterator end) -> Future<utils::Result<uintptr_t>> {
    using CoOutput = utils::Result<uintptr_t>;

    class Future : public runtime::Future<CoOutput> {
     public:
      auto poll(runtime::Handle<void> self)
          -> runtime::Poll<CoOutput> override {
        auto *extra =
            dynamic_cast<io::epoll::IoExtra *>(self_->event_->extra_.get());

        if (!extra->state_.get_field<io::epoll::IoExtra::State::Registered>()) {
          self_->event_->future_ = this;
          extra->interest_ = io::epoll::IoExtra::Interest::Read;
          runtime::RuntimeCtx::get_ctx()
              ->driver()
              .Register<io::epoll::IoRegistry>(self_->event_);
          TRACE("register read {}", self_->socket_);
          return runtime::Pending();
        }
        if (extra->state_.get_field<io::epoll::IoExtra::State::Error>() ||
            extra->state_.get_field<io::epoll::IoExtra::State::Readable>()) {
          auto read_bytes = ::read(self_->socket_.into_c_fd(), &*begin_,
                                   std::distance(begin_, end_));
          if (read_bytes != -1) {
            INFO("read {} bytes from {}", read_bytes, *self_);
            extra->state_
                .set_field<io::epoll::IoExtra::State::Readable, false>();
            return runtime::Ready<CoOutput>{read_bytes};
          }
          if (errno != EAGAIN && errno != EWOULDBLOCK) {
            return runtime::Ready<CoOutput>{
                utils::into_sys_result(-1).transform(
                    [](auto value) { return -1; })};
          }
        }
        self_->event_->future_ = this;
        extra->interest_ = io::epoll::IoExtra::Interest::Read;
        TRACE("reregister read {}", self_->socket_);
        runtime::RuntimeCtx::get_ctx()
            ->driver()
            .reregister<io::epoll::IoRegistry>(self_->event_);
        return runtime::Pending();
      }

      Future(Iterator begin, Iterator end, net::epoll::TcpStream *self)
          : runtime::Future<CoOutput>(nullptr),
            begin_(begin),
            end_(end),
            self_(self) {}

      // NOLINTNEXTLINE(cppcoreguidelines-avoid-reference-coroutine-parameters)
      Future(const Future &future) = delete;

      // NOLINTNEXTLINE(cppcoreguidelines-avoid-reference-coroutine-parameters)
      Future(Future &&future) = delete;

      // NOLINTNEXTLINE(cppcoreguidelines-avoid-reference-coroutine-parameters)
      auto operator=(Future &&future) -> Future & = delete;

      // NOLINTNEXTLINE(cppcoreguidelines-avoid-reference-coroutine-parameters)
      auto operator=(const Future &future) -> Future & = delete;

      ~Future() override { self_->event_->future_ = nullptr; }

     private:
      net::epoll::TcpStream *self_;
      Iterator begin_;
      Iterator end_;
    };

    co_return co_await Future(begin, end, this);
  }

  template <typename Iterator>
  auto write(Iterator begin, Iterator end) -> Future<utils::Result<uintptr_t>> {
    using CoOutput = utils::Result<uintptr_t>;

    class Future : public runtime::Future<CoOutput> {
     public:
      auto poll(runtime::Handle<void> self)
          -> runtime::Poll<CoOutput> override {
        auto *extra =
            dynamic_cast<io::epoll::IoExtra *>(self_->event_->extra_.get());

        if (!extra->state_.get_field<io::epoll::IoExtra::State::Registered>()) {
          self_->event_->future_ = this;
          extra->interest_ = io::epoll::IoExtra::Interest::Write;
          runtime::RuntimeCtx::get_ctx()
              ->driver()
              .Register<io::epoll::IoRegistry>(self_->event_);
          return runtime::Pending();
        }
        if (extra->state_.get_field<io::epoll::IoExtra::State::Error>() ||
            extra->state_.get_field<io::epoll::IoExtra::State::Writable>()) {
          auto write_bytes = ::write(self_->socket_.into_c_fd(), &*begin_,
                                     std::distance(begin_, end_));
          auto nbytes =
              utils::into_sys_result(write_bytes).transform([](auto n) {
                return static_cast<uintptr_t>(n);
              });
          INFO("write {} bytes to {}", write_bytes, self_->socket_);
          return runtime::Ready<CoOutput>{nbytes};
        }
        self_->event_->future_ = this;
        extra->interest_ = io::epoll::IoExtra::Interest::Write;
        runtime::RuntimeCtx::get_ctx()
            ->driver()
            .reregister<io::epoll::IoRegistry>(self_->event_);
        return runtime::Pending();
      }

      Future(Iterator begin, Iterator end, TcpStream *self)
          : runtime::Future<CoOutput>(nullptr),
            begin_(begin),
            end_(end),
            self_(self) {}

      // NOLINTNEXTLINE(cppcoreguidelines-avoid-reference-coroutine-parameters)
      Future(const Future &future) = delete;

      // NOLINTNEXTLINE(cppcoreguidelines-avoid-reference-coroutine-parameters)
      Future(Future &&future) = delete;

      // NOLINTNEXTLINE(cppcoreguidelines-avoid-reference-coroutine-parameters)
      auto operator=(Future &&future) -> Future & = delete;

      // NOLINTNEXTLINE(cppcoreguidelines-avoid-reference-coroutine-parameters)
      auto operator=(const Future &future) -> Future & = delete;

      ~Future() override { self_->event_->future_ = nullptr; }

     private:
      TcpStream *self_;
      Iterator begin_;
      Iterator end_;
    };

    co_return co_await Future(begin, end, this);
  }

  auto flush() -> Future<utils::Result<void>>;

  [[nodiscard]] auto shutdown(io::Shutdown shutdown) const
      -> Future<utils::Result<void>>;

  TcpStream(const TcpStream &tcp_stream) = delete;

  TcpStream(TcpStream &&tcp_stream) noexcept = default;

  auto operator=(const TcpStream &tcp_stream) -> TcpStream & = delete;

  auto operator=(TcpStream &&tcp_stream) noexcept -> TcpStream & = default;

  ~TcpStream();

 private:
  explicit TcpStream(Socket &&socket, bool writable = false,
                     bool readable = false);

  Socket socket_;
  std::shared_ptr<runtime::Event> event_;
};

class TcpListener {
  friend class TcpSocket;
  friend struct std::formatter<TcpListener>;

  template <typename T>
  using Future = runtime::Future<T>;

 public:
  static auto bind(SocketAddr addr) -> Future<utils::Result<TcpListener>>;

  auto accept() -> Future<utils::Result<std::pair<TcpStream, SocketAddr>>>;

  TcpListener(const TcpListener &tcp_listener) = delete;

  TcpListener(TcpListener &&tcp_listener) noexcept = default;

  auto operator=(const TcpListener &tcp_listener) -> TcpListener & = delete;

  auto operator=(TcpListener &&tcp_listener) noexcept
      -> TcpListener & = default;

  ~TcpListener();

 private:
  TcpListener(Socket &&socket);

  Socket socket_;
  std::shared_ptr<runtime::Event> event_;
};
}  // namespace xyco::net::epoll

template <>
struct std::formatter<xyco::net::epoll::TcpSocket>
    : public std::formatter<bool> {
  template <typename FormatContext>
  auto format(const xyco::net::epoll::TcpSocket &tcp_socket,
              FormatContext &ctx) const -> decltype(ctx.out()) {
    return std::format_to(ctx.out(), "TcpSocket{{socket_={}}}",
                          tcp_socket.socket_);
  }
};

template <>
struct std::formatter<xyco::net::epoll::TcpStream>
    : public std::formatter<bool> {
  template <typename FormatContext>
  auto format(const xyco::net::epoll::TcpStream &tcp_stream,
              FormatContext &ctx) const -> decltype(ctx.out()) {
    return std::format_to(ctx.out(), "TcpStream{{socket_={}}}",
                          tcp_stream.socket_);
  }
};

template <>
struct std::formatter<xyco::net::epoll::TcpListener>
    : public std::formatter<bool> {
  template <typename FormatContext>
  auto format(const xyco::net::epoll::TcpListener &tcp_listener,
              FormatContext &ctx) const -> decltype(ctx.out()) {
    return std::format_to(ctx.out(), "TcpListener{{socket_={}}}",
                          tcp_listener.socket_);
  }
};

#endif  // XYCO_NET_EPOLL_LISTENER_H_
