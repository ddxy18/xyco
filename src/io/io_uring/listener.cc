#include "listener.h"

#include <arpa/inet.h>

#include <cerrno>
#include <clocale>
#include <cstdint>
#include <variant>

#include "extra.h"
#include "io_uring.h"
#include "runtime/async_future.h"
#include "runtime/registry.h"

template <typename T>
using Future = xyco::runtime::Future<T>;

auto xyco::net::uring::TcpSocket::bind(SocketAddr addr)
    -> Future<io::IoResult<void>> {
  auto bind_result = co_await runtime::AsyncFuture<io::IoResult<int>>([&]() {
    return io::into_sys_result(
        ::bind(socket_.into_c_fd(), addr.into_c_addr(), sizeof(sockaddr)));
  });
  if (bind_result.is_ok()) {
    INFO("{} bind to {}", socket_, addr);
  }

  co_return bind_result;
}

auto xyco::net::uring::TcpSocket::connect(SocketAddr addr)
    -> Future<io::IoResult<TcpStream>> {
  using CoOutput = io::IoResult<TcpStream>;

  class Future : public runtime::Future<CoOutput> {
   public:
    explicit Future(SocketAddr addr, Socket &socket)
        : runtime::Future<CoOutput>(nullptr),
          socket_(socket),
          addr_(addr),
          event_(std::make_shared<runtime::Event>(runtime::Event{
              .extra_ = std::make_unique<io::uring::IoExtra>()})) {}

    auto poll(runtime::Handle<void> self) -> runtime::Poll<CoOutput> override {
      auto *extra = dynamic_cast<io::uring::IoExtra *>(event_->extra_.get());
      if (!extra->state_.get_field<io::uring::IoExtra::State::Registered>()) {
        event_->future_ = this;
        extra->args_ = io::uring::IoExtra::Connect{
            .addr_ = addr_.into_c_addr(), .addrlen_ = sizeof(sockaddr)};
        extra->fd_ = socket_.into_c_fd();
        runtime::RuntimeCtx::get_ctx()->driver().Register<io::IoUringRegistry>(
            event_);

        return runtime::Pending();
      }

      extra->state_.set_field<io::uring::IoExtra::State::Registered, false>();
      if (extra->return_ < 0) {
        return runtime::Ready<CoOutput>{
            CoOutput::err(io::IoError{.errno_ = -extra->return_})};
      }
      INFO("{} connect to {}", socket_, addr_);
      return runtime::Ready<CoOutput>{
          CoOutput::ok(TcpStream(std::move(socket_)))};
    }

   private:
    Socket &socket_;
    SocketAddr addr_;
    std::shared_ptr<runtime::Event> event_;
  };

  co_return co_await Future(addr, socket_);
}

auto xyco::net::uring::TcpSocket::listen(int backlog)
    -> Future<io::IoResult<TcpListener>> {
  auto listen_result = co_await runtime::AsyncFuture<io::IoResult<int>>([&]() {
    return io::into_sys_result(::listen(socket_.into_c_fd(), backlog));
  });
  ASYNC_TRY(listen_result.map([&](auto n) { return TcpListener(Socket(-1)); }));
  INFO("{} listening", socket_);

  co_return io::IoResult<TcpListener>::ok(TcpListener(std::move(socket_)));
}

auto xyco::net::uring::TcpSocket::set_reuseaddr(bool reuseaddr)
    -> io::IoResult<void> {
  int optval = static_cast<int>(reuseaddr);
  return io::into_sys_result(::setsockopt(
      socket_.into_c_fd(), SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval)));
}

auto xyco::net::uring::TcpSocket::set_reuseport(bool reuseport)
    -> io::IoResult<void> {
  int optval = static_cast<int>(reuseport);
  return io::into_sys_result(::setsockopt(
      socket_.into_c_fd(), SOL_SOCKET, SO_REUSEPORT, &optval, sizeof(optval)));
}

auto xyco::net::uring::TcpSocket::new_v4() -> io::IoResult<TcpSocket> {
  return io::into_sys_result(::socket(AF_INET, SOCK_STREAM, 0))
      .map([](auto fd) { return TcpSocket(fd); });
}

auto xyco::net::uring::TcpSocket::new_v6() -> io::IoResult<TcpSocket> {
  return io::into_sys_result(::socket(AF_INET6, SOCK_STREAM, 0))
      .map([](auto fd) { return TcpSocket(fd); });
}

xyco::net::uring::TcpSocket::TcpSocket(Socket &&socket)
    : socket_(std::move(socket)) {}

auto xyco::net::uring::TcpStream::connect(SocketAddr addr)
    -> Future<io::IoResult<TcpStream>> {
  auto socket = addr.is_v4() ? TcpSocket::new_v4() : TcpSocket::new_v6();
  if (socket.is_err()) {
    co_return io::IoResult<TcpStream>::err(socket.unwrap_err());
  }
  co_return co_await socket.unwrap().connect(addr);
}

auto xyco::net::uring::TcpStream::flush() -> Future<io::IoResult<void>> {
  co_return io::IoResult<void>::ok();
}

auto xyco::net::uring::TcpStream::shutdown(io::Shutdown shutdown)
    -> Future<io::IoResult<void>> {
  using CoOutput = io::IoResult<void>;

  class Future : public runtime::Future<CoOutput> {
   public:
    explicit Future(io::Shutdown shutdown, TcpStream *tcp_stream)
        : runtime::Future<CoOutput>(nullptr),
          shutdown_(shutdown),
          self_(tcp_stream) {}

    auto poll(runtime::Handle<void> self) -> runtime::Poll<CoOutput> override {
      auto *extra =
          dynamic_cast<io::uring::IoExtra *>(self_->event_->extra_.get());
      if (!extra->state_.get_field<io::uring::IoExtra::State::Registered>()) {
        self_->event_->future_ = this,
        extra->args_ = io::uring::IoExtra::Shutdown{.shutdown_ = shutdown_};
        extra->fd_ = self_->socket_.into_c_fd();
        runtime::RuntimeCtx::get_ctx()->driver().Register<io::IoUringRegistry>(
            self_->event_);

        return runtime::Pending();
      }

      extra->state_.set_field<io::uring::IoExtra::State::Registered, false>();
      if (extra->return_ < 0) {
        return runtime::Ready<CoOutput>{
            CoOutput::err(io::IoError{.errno_ = -extra->return_})};
      }
      INFO("shutdown {}", self_->socket_);
      return runtime::Ready<CoOutput>{CoOutput::ok()};
    }

   private:
    TcpStream *self_;
    io::Shutdown shutdown_;
  };

  auto result = co_await Future(shutdown, this);
  event_->future_ = nullptr;
  if (result.is_err() && result.unwrap_err().errno_ ==
                             EINVAL) {  // io_uring shutdown is not supported
    result = co_await runtime::AsyncFuture<io::IoResult<int>>([&]() {
      return io::into_sys_result(::shutdown(
          socket_.into_c_fd(),
          static_cast<std::underlying_type_t<io::Shutdown>>(shutdown)));
    });
  }
  if (result.is_ok()) {
    INFO("shutdown {}", socket_);
  }
  co_return result;
}

xyco::net::uring::TcpStream::~TcpStream() {
  if (socket_.into_c_fd() != -1 &&
      dynamic_cast<io::uring::IoExtra *>(event_->extra_.get())->fd_ != 0) {
    /* runtime::RuntimeCtx::get_ctx()->driver().deregister<io::IoUringRegistry>(
        event_); */
  }
}

xyco::net::uring::TcpStream::TcpStream(Socket &&socket, bool writable,
                                       bool readable)
    : socket_(std::move(socket)),
      event_(std::make_shared<runtime::Event>(
          runtime::Event{.extra_ = std::make_unique<io::uring::IoExtra>()})) {
  auto *extra = dynamic_cast<io::uring::IoExtra *>(event_->extra_.get());
  extra->fd_ = socket_.into_c_fd();
}

auto xyco::net::uring::TcpListener::bind(SocketAddr addr)
    -> Future<io::IoResult<TcpListener>> {
  using runtime::Handle;
  using runtime::Ready;

  const int max_pending_connection = 128;

  auto socket_result = TcpSocket::new_v4();
  if (socket_result.is_err()) {
    co_return io::IoResult<TcpListener>::err(socket_result.unwrap_err());
  }
  auto tcp_socket = socket_result.unwrap();
  auto bind_result = co_await tcp_socket.bind(addr);
  if (bind_result.is_err()) {
    co_return io::IoResult<TcpListener>::err(bind_result.unwrap_err());
  }
  co_return co_await tcp_socket.listen(max_pending_connection);
}

auto xyco::net::uring::TcpListener::accept()
    -> Future<io::IoResult<std::pair<TcpStream, SocketAddr>>> {
  using CoOutput = io::IoResult<std::pair<TcpStream, SocketAddr>>;

  class Future : public runtime::Future<CoOutput> {
   public:
    auto poll(runtime::Handle<void> self) -> runtime::Poll<CoOutput> override {
      auto *extra =
          dynamic_cast<io::uring::IoExtra *>(self_->event_->extra_.get());
      if (!extra->state_.get_field<io::uring::IoExtra::State::Registered>()) {
        self_->event_->future_ = this;
        extra->args_ = io::uring::IoExtra::Accept{
            .addr_ = static_cast<sockaddr *>(static_cast<void *>(&addr_)),
            .addrlen_ = &addr_len_};
        extra->fd_ = self_->socket_.into_c_fd();
        runtime::RuntimeCtx::get_ctx()->driver().Register<io::IoUringRegistry>(
            self_->event_);

        return runtime::Pending();
      }

      if (extra->return_ < 0) {
        return runtime::Ready<CoOutput>{
            CoOutput::err(io::IoError{.errno_ = -extra->return_})};
      }
      std::string ip(INET_ADDRSTRLEN, 0);
      auto sock_addr = SocketAddr::new_v4(
          Ipv4Addr(::inet_ntop(addr_.sin_family, &addr_.sin_addr, ip.data(),
                               ip.size())),
          addr_.sin_port);
      auto socket = Socket(extra->return_);
      INFO("accept from {} new connect={{{}, addr:{}}}", self_->socket_, socket,
           sock_addr);
      extra->state_.set_field<io::uring::IoExtra::State::Registered, false>();
      return runtime::Ready<CoOutput>{
          CoOutput::ok(TcpStream(std::move(socket)), sock_addr)};
    }

    explicit Future(TcpListener *self)
        : runtime::Future<CoOutput>(nullptr), self_(self) {}

    Future(const Future &future) = delete;

    Future(Future &&future) = delete;

    auto operator=(Future &&future) -> Future & = delete;

    auto operator=(const Future &future) -> Future & = delete;

    ~Future() override { self_->event_->future_ = nullptr; }

   private:
    TcpListener *self_;
    sockaddr_in addr_{};
    socklen_t addr_len_{sizeof(addr_)};
  };

  co_return co_await Future(this);
}

xyco::net::uring::TcpListener::~TcpListener() {
  if (socket_.into_c_fd() != -1 && event_ != nullptr &&
      dynamic_cast<io::uring::IoExtra *>(event_->extra_.get())->fd_ != 0) {
    runtime::RuntimeCtx::get_ctx()->driver().deregister<io::IoUringRegistry>(
        event_);
  }
}

xyco::net::uring::TcpListener::TcpListener(Socket &&socket)
    : socket_(std::move(socket)),
      event_(std::make_shared<runtime::Event>(
          runtime::Event{.extra_ = std::make_unique<io::uring::IoExtra>()})) {}

template <typename FormatContext>
auto fmt::formatter<xyco::net::uring::TcpSocket>::format(
    const xyco::net::uring::TcpSocket &tcp_socket, FormatContext &ctx) const
    -> decltype(ctx.out()) {
  return format_to(ctx.out(), "TcpSocket{{socket_={}}}", tcp_socket.socket_);
}

template <typename FormatContext>
auto fmt::formatter<xyco::net::uring::TcpStream>::format(
    const xyco::net::uring::TcpStream &tcp_stream, FormatContext &ctx) const
    -> decltype(ctx.out()) {
  return format_to(ctx.out(), "TcpStream{{socket_={}}}", tcp_stream.socket_);
}

template <typename FormatContext>
auto fmt::formatter<xyco::net::uring::TcpListener>::format(
    const xyco::net::uring::TcpListener &tcp_listener, FormatContext &ctx) const
    -> decltype(ctx.out()) {
  return format_to(ctx.out(), "TcpListener{{socket_={}}}",
                   tcp_listener.socket_);
}

template auto fmt::formatter<xyco::net::uring::TcpSocket>::format(
    const xyco::net::uring::TcpSocket &tcp_socket,
    fmt::basic_format_context<fmt::appender, char> &ctx) const
    -> decltype(ctx.out());

template auto fmt::formatter<xyco::net::uring::TcpStream>::format(
    const xyco::net::uring::TcpStream &tcp_stream,
    fmt::basic_format_context<fmt::appender, char> &ctx) const
    -> decltype(ctx.out());

template auto fmt::formatter<xyco::net::uring::TcpListener>::format(
    const xyco::net::uring::TcpListener &tcp_listener,
    fmt::basic_format_context<fmt::appender, char> &ctx) const
    -> decltype(ctx.out());