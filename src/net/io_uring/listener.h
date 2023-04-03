#ifndef XYCO_NET_IO_URING_LISTENER_H_
#define XYCO_NET_IO_URING_LISTENER_H_

#include <unistd.h>

#include "io/io_uring/extra.h"
#include "io/io_uring/registry.h"
#include "net/socket.h"
#include "runtime/runtime.h"

namespace xyco::net::uring {
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
            dynamic_cast<io::uring::IoExtra *>(self_->event_->extra_.get());

        if (!extra->state_.get_field<io::uring::IoExtra::State::Completed>()) {
          self_->event_->future_ = this;
          extra->args_ = io::uring::IoExtra::Read{
              .buf_ = &*begin_,
              .len_ = static_cast<unsigned int>(std::distance(begin_, end_))};
          runtime::RuntimeCtx::get_ctx()
              ->driver()
              .Register<io::uring::IoRegistry>(self_->event_);
          TRACE("register read {}", self_->socket_);
          return runtime::Pending();
        }
        extra->state_.set_field<io::uring::IoExtra::State::Completed, false>();
        if (extra->return_ >= 0) {
          INFO("read {} bytes from {}", extra->return_, self_->socket_);
          return runtime::Ready<CoOutput>{CoOutput::ok(extra->return_)};
        }
        return runtime::Ready<CoOutput>{
            CoOutput::err(utils::Error{.errno_ = -extra->return_})};
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

  template <typename Iterator>
  auto write(Iterator begin, Iterator end) -> Future<utils::Result<uintptr_t>> {
    using CoOutput = utils::Result<uintptr_t>;

    class Future : public runtime::Future<CoOutput> {
     public:
      auto poll(runtime::Handle<void> self)
          -> runtime::Poll<CoOutput> override {
        auto *extra =
            dynamic_cast<io::uring::IoExtra *>(self_->event_->extra_.get());
        if (!extra->state_.get_field<io::uring::IoExtra::State::Completed>()) {
          self_->event_->future_ = this;
          extra->args_ = io::uring::IoExtra::Write{
              .buf_ = &*begin_,
              .len_ = static_cast<unsigned int>(std::distance(begin_, end_))};
          runtime::RuntimeCtx::get_ctx()
              ->driver()
              .Register<io::uring::IoRegistry>(self_->event_);

          return runtime::Pending();
        }
        extra->state_.set_field<io::uring::IoExtra::State::Completed, false>();
        if (extra->return_ >= 0) {
          INFO("write {} bytes to {}", extra->return_, self_->socket_);
          return runtime::Ready<CoOutput>{CoOutput::ok(extra->return_)};
        }
        return runtime::Ready<CoOutput>{
            CoOutput::err(utils::Error{.errno_ = -extra->return_})};
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

  static auto flush() -> Future<utils::Result<void>>;

  [[nodiscard]] auto shutdown(io::Shutdown shutdown)
      -> Future<utils::Result<void>>;

  TcpStream(const TcpStream &tcp_stream) = delete;

  TcpStream(TcpStream &&tcp_stream) noexcept = default;

  auto operator=(const TcpStream &tcp_stream) -> TcpStream & = delete;

  auto operator=(TcpStream &&tcp_stream) noexcept -> TcpStream & = default;

  ~TcpStream() = default;

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

  ~TcpListener() = default;

 private:
  TcpListener(Socket &&socket);

  Socket socket_;
  std::shared_ptr<runtime::Event> event_;
};
}  // namespace xyco::net::uring

template <>
struct std::formatter<xyco::net::uring::TcpSocket>
    : public std::formatter<bool> {
  template <typename FormatContext>
  auto format(const xyco::net::uring::TcpSocket &tcp_socket,
              FormatContext &ctx) const -> decltype(ctx.out()) {
    return std::format_to(ctx.out(), "TcpSocket{{socket_={}}}",
                          tcp_socket.socket_);
  }
};

template <>
struct std::formatter<xyco::net::uring::TcpStream>
    : public std::formatter<bool> {
  template <typename FormatContext>
  auto format(const xyco::net::uring::TcpStream &tcp_stream,
              FormatContext &ctx) const -> decltype(ctx.out()) {
    return std::format_to(ctx.out(), "TcpStream{{socket_={}}}",
                          tcp_stream.socket_);
  }
};

template <>
struct std::formatter<xyco::net::uring::TcpListener>
    : public std::formatter<bool> {
  template <typename FormatContext>
  auto format(const xyco::net::uring::TcpListener &tcp_listener,
              FormatContext &ctx) const -> decltype(ctx.out()) {
    return std::format_to(ctx.out(), "TcpListener{{socket_={}}}",
                          tcp_listener.socket_);
  }
};

#endif  // XYCO_NET_IO_URING_LISTENER_H_
