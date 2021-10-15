#include "listener.h"

#include <arpa/inet.h>

#include <cerrno>
#include <clocale>
#include <cstdint>

#include "runtime/async_future.h"

template <typename T>
using Future = xyco::runtime::Future<T>;

auto xyco::net::TcpSocket::bind(SocketAddr addr) -> Future<io::IoResult<void>> {
  auto bind_result = co_await runtime::AsyncFuture<io::IoResult<int>>([&]() {
    return io::into_sys_result(
        ::bind(socket_.into_c_fd(), addr.into_c_addr(), sizeof(sockaddr)));
  });
  if (bind_result.is_ok()) {
    INFO("{} bind to {}", socket_, addr);
  }

  co_return bind_result;
}

auto xyco::net::TcpSocket::connect(SocketAddr addr)
    -> Future<io::IoResult<TcpStream>> {
  using CoOutput = io::IoResult<TcpStream>;

  class Future : public runtime::Future<CoOutput> {
   public:
    explicit Future(SocketAddr addr, Socket &socket)
        : runtime::Future<CoOutput>(nullptr), socket_(socket), addr_(addr) {}

    auto poll(runtime::Handle<void> self) -> runtime::Poll<CoOutput> override {
      if (event_.future_ == nullptr) {
        event_ = runtime::Event{
            .future_ = this,
            .extra_ = runtime::IoExtra{.fd_ = socket_.into_c_fd()}};
        auto register_result =
            runtime::RuntimeCtx::get_ctx()->io_handle()->Register(
                event_, runtime::Interest::All);
        if (register_result.is_err()) {
          return runtime::Ready<CoOutput>{
              CoOutput::err(register_result.unwrap_err())};
        }
        return runtime::Pending();
      }
      int ret = -1;
      socklen_t len = sizeof(decltype(ret));
      getsockopt(socket_.into_c_fd(), SOL_SOCKET, SO_ERROR, &ret, &len);
      if (ret != 0) {
        return runtime::Ready<CoOutput>{CoOutput::err(
            io::IoError{ret, strerror_l(ret, uselocale(nullptr))})};
      }
      INFO("{} connect to {}", socket_, addr_);
      return runtime::Ready<CoOutput>{CoOutput::ok(
          TcpStream(std::move(socket_),
                    std::get<runtime::IoExtra>(event_.extra_).state_))};
    }

   private:
    Socket &socket_;
    SocketAddr addr_;
    runtime::Event event_;
  };

  auto connect_result = co_await runtime::AsyncFuture<io::IoResult<int>>([&]() {
    return io::into_sys_result(
        ::connect(socket_.into_c_fd(), addr.into_c_addr(), sizeof(sockaddr)));
  });
  if (connect_result.is_ok()) {
    co_return CoOutput::ok(
        TcpStream(std::move(socket_), runtime::IoExtra::State::Writable));
  }
  auto err = connect_result.unwrap_err().errno_;
  if (err != EINPROGRESS && err != EAGAIN) {
    WARN("{} connect fail{{errno={}}}", socket_, errno);
    co_return CoOutput::err(io::IoError{.errno_ = err});
  }
  co_return co_await Future(addr, socket_);
}

auto xyco::net::TcpSocket::listen(int backlog)
    -> Future<io::IoResult<TcpListener>> {
  auto listen_result = co_await runtime::AsyncFuture<io::IoResult<int>>([&]() {
    return io::into_sys_result(::listen(socket_.into_c_fd(), backlog));
  });
  ASYNC_TRY(listen_result.map([&](auto n) { return TcpListener(Socket(-1)); }));
  INFO("{} listening", socket_);

  co_return io::IoResult<TcpListener>::ok(TcpListener(std::move(socket_)));
}

auto xyco::net::TcpSocket::set_reuseaddr(bool reuseaddr) -> io::IoResult<void> {
  return io::into_sys_result(::setsockopt(
      socket_.into_c_fd(), SOL_SOCKET, SO_REUSEADDR, &reuseaddr, sizeof(bool)));
}

auto xyco::net::TcpSocket::set_reuseport(bool reuseport) -> io::IoResult<void> {
  return io::into_sys_result(::setsockopt(
      socket_.into_c_fd(), SOL_SOCKET, SO_REUSEADDR, &reuseport, sizeof(bool)));
}

auto xyco::net::TcpSocket::new_v4() -> io::IoResult<TcpSocket> {
  return io::into_sys_result(::socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0))
      .map([](auto fd) { return TcpSocket(fd); });
}

auto xyco::net::TcpSocket::new_v6() -> io::IoResult<TcpSocket> {
  return io::into_sys_result(::socket(AF_INET6, SOCK_STREAM | SOCK_NONBLOCK, 0))
      .map([](auto fd) { return TcpSocket(fd); });
}

xyco::net::TcpSocket::TcpSocket(Socket &&socket) : socket_(std::move(socket)) {}

auto xyco::net::TcpStream::connect(SocketAddr addr)
    -> Future<io::IoResult<TcpStream>> {
  auto socket = addr.is_v4() ? TcpSocket::new_v4() : TcpSocket::new_v6();
  if (socket.is_err()) {
    co_return io::IoResult<TcpStream>::err(socket.unwrap_err());
  }
  co_return co_await socket.unwrap().connect(addr);
}

auto xyco::net::TcpStream::flush() -> Future<io::IoResult<void>> {
  co_return io::IoResult<void>::ok();
}

auto xyco::net::TcpStream::shutdown(io::Shutdown shutdown) const
    -> Future<io::IoResult<void>> {
  ASYNC_TRY(co_await runtime::AsyncFuture<io::IoResult<int>>([&]() {
    return io::into_sys_result(::shutdown(
        socket_.into_c_fd(),
        static_cast<std::underlying_type_t<io::Shutdown>>(shutdown)));
  }));
  INFO("shutdown {}", socket_);

  co_return io::IoResult<void>::ok();
}

xyco::net::TcpStream::~TcpStream() {
  if (socket_.into_c_fd() != -1) {
    runtime::RuntimeCtx::get_ctx()
        ->io_handle()
        ->deregister_local(*event_, runtime::Interest::All)
        .unwrap();
  }
}

xyco::net::TcpStream::TcpStream(Socket &&socket, xyco::runtime::IoExtra::State state)
    : socket_(std::move(socket)),
      event_(std::make_unique<runtime::Event>(
          runtime::Event{.extra_ = runtime::IoExtra{
                             .state_ = state, .fd_ = socket_.into_c_fd()}})) {
  auto *io_handle = runtime::RuntimeCtx::get_ctx()->io_handle();
  auto register_result =
      io_handle->register_local(*event_, runtime::Interest::All);
  if (register_result.is_err()) {
    auto err = register_result.unwrap_err().errno_;
    if (err == EEXIST) {
      io_handle->reregister_local(*event_, runtime::Interest::All).unwrap();
    }
  }
}

auto xyco::net::TcpListener::bind(SocketAddr addr)
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

auto xyco::net::TcpListener::accept()
    -> Future<io::IoResult<std::pair<TcpStream, SocketAddr>>> {
  using CoOutput = io::IoResult<std::pair<TcpStream, SocketAddr>>;

  class Future : public runtime::Future<CoOutput> {
   public:
    explicit Future(TcpListener *self)
        : runtime::Future<CoOutput>(nullptr), self_(self) {}

    auto poll(runtime::Handle<void> self) -> runtime::Poll<CoOutput> override {
      if (self_->event_ == nullptr) {
        self_->event_ = std::make_unique<runtime::Event>(runtime::Event{
            .future_ = this,
            .extra_ = runtime::IoExtra{.fd_ = self_->socket_.into_c_fd()}});
        auto res = runtime::RuntimeCtx::get_ctx()->io_handle()->Register(
            *self_->event_, runtime::Interest::Read);
        if (res.is_err()) {
          return runtime::Ready<CoOutput>{CoOutput::err(res.unwrap_err())};
        }
      }

      auto extra = std::get<runtime::IoExtra>(self_->event_->extra_);
      if (extra.readable()) {
        sockaddr_in addr_in{};
        socklen_t addrlen = sizeof(addr_in);
        auto accept_result = io::into_sys_result(
            ::accept4(self_->socket_.into_c_fd(),
                      static_cast<sockaddr *>(static_cast<void *>(&addr_in)),
                      &addrlen, SOCK_NONBLOCK));
        if (accept_result.is_err()) {
          auto err = accept_result.unwrap_err();
          if (err.errno_ == EAGAIN || err.errno_ == EWOULDBLOCK) {
            extra.state_ = runtime::IoExtra::State::Pending;
            return runtime::Pending();
          }
          return runtime::Ready<CoOutput>{CoOutput::err(err)};
        }
        std::string ip(INET_ADDRSTRLEN, 0);
        auto sock_addr = SocketAddr::new_v4(
            Ipv4Addr(inet_ntop(addr_in.sin_family, &addr_in.sin_addr, ip.data(),
                               ip.size())),
            addr_in.sin_port);
        auto socket = Socket(accept_result.unwrap());
        INFO("accept from {} new connect={{{}, addr:{}}}", self_->socket_,
             socket, sock_addr);
        return runtime::Ready<CoOutput>{CoOutput::ok(
            TcpStream(std::move(socket), runtime::IoExtra::State::Writable),
            sock_addr)};
      }
      return runtime::Pending();
    }

   private:
    TcpListener *self_;
  };

  auto res = co_await Future(this);
  co_return res;
}

xyco::net::TcpListener::~TcpListener() {
  if (socket_.into_c_fd() != -1 && event_ != nullptr) {
    runtime::RuntimeCtx::get_ctx()
        ->io_handle()
        ->deregister(*event_, runtime::Interest::All)
        .unwrap();
  }
}

xyco::net::TcpListener::TcpListener(Socket &&socket)
    : socket_(std::move(socket)) {}

template <typename FormatContext>
auto fmt::formatter<xyco::net::TcpSocket>::format(
    const xyco::net::TcpSocket &tcp_socket, FormatContext &ctx) const
    -> decltype(ctx.out()) {
  return format_to(ctx.out(), "TcpSocket{{socket_={}}}", tcp_socket.socket_);
}

template <typename FormatContext>
auto fmt::formatter<xyco::net::TcpStream>::format(
    const xyco::net::TcpStream &tcp_stream, FormatContext &ctx) const
    -> decltype(ctx.out()) {
  return format_to(ctx.out(), "TcpStream{{socket_={}}}", tcp_stream.socket_);
}

template <typename FormatContext>
auto fmt::formatter<xyco::net::TcpListener>::format(
    const xyco::net::TcpListener &tcp_listener, FormatContext &ctx) const
    -> decltype(ctx.out()) {
  return format_to(ctx.out(), "TcpListener{{socket_={}}}",
                   tcp_listener.socket_);
}

template auto fmt::formatter<xyco::net::TcpSocket>::format(
    const xyco::net::TcpSocket &tcp_socket,
    fmt::basic_format_context<fmt::appender, char> &ctx) const
    -> decltype(ctx.out());

template auto fmt::formatter<xyco::net::TcpStream>::format(
    const xyco::net::TcpStream &tcp_stream,
    fmt::basic_format_context<fmt::appender, char> &ctx) const
    -> decltype(ctx.out());

template auto fmt::formatter<xyco::net::TcpListener>::format(
    const xyco::net::TcpListener &tcp_listener,
    fmt::basic_format_context<fmt::appender, char> &ctx) const
    -> decltype(ctx.out());