#include "listener.h"

#include <arpa/inet.h>
#include <unistd.h>

#include <cerrno>
#include <clocale>
#include <cstdint>

#include "event/runtime/async.h"

template <typename T>
using Future = runtime::Future<T>;

auto net::TcpSocket::bind(SocketAddr addr) -> Future<IoResult<void>> {
  auto bind = co_await runtime::AsyncFuture<int>([&]() {
    return ::bind(socket_.into_c_fd(), addr.into_c_addr(), sizeof(sockaddr));
  });
  auto res = into_sys_result(bind).map([](auto n) {});
  ASYNC_TRY(res);
  INFO("{} bind to {}\n", socket_, addr);

  co_return res;
}

auto net::TcpSocket::connect(SocketAddr addr) -> Future<IoResult<TcpStream>> {
  using CoOutput = IoResult<TcpStream>;

  class Future : public runtime::Future<CoOutput> {
   public:
    explicit Future(SocketAddr addr, Socket socket)
        : runtime::Future<CoOutput>(nullptr), socket_(socket), addr_(addr) {}

    auto poll(runtime::Handle<void> self) -> runtime::Poll<CoOutput> override {
      if (event_.fd_ == 0) {
        event_ = reactor::Event{.fd_ = socket_.into_c_fd(), .future_ = this};
        auto register_result =
            runtime::RuntimeCtx::get_ctx()->io_handle()->Register(
                event_, reactor::Interest::All);
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
        return runtime::Ready<CoOutput>{
            CoOutput::err(IoError{ret, strerror_l(ret, uselocale(nullptr))})};
      }
      INFO("{} connect to {}\n", socket_, addr_);
      return runtime::Ready<CoOutput>{
          CoOutput::ok(TcpStream(socket_, event_.state_))};
    }

   private:
    Socket socket_;
    SocketAddr addr_;
    reactor::Event event_;
  };

  auto connect_result = co_await runtime::AsyncFuture<IoResult<int>>([&]() {
    return into_sys_result(
        ::connect(socket_.into_c_fd(), addr.into_c_addr(), sizeof(sockaddr)));
  });
  if (connect_result.is_ok()) {
    co_return CoOutput::ok(TcpStream(socket_, reactor::Event::State::Writable));
  }
  auto err = connect_result.unwrap_err().errno_;
  if (err != EINPROGRESS && err != EAGAIN) {
    WARN("{} connect fail{{errno={}}}", socket_, errno);
    co_return CoOutput::err(IoError{.errno_ = err});
  }
  co_return co_await Future(addr, socket_);
}

auto net::TcpSocket::listen(int backlog) -> Future<IoResult<TcpListener>> {
  auto listener = TcpListener(socket_.into_c_fd());

  auto listen = co_await runtime::AsyncFuture<int>(
      [&]() { return ::listen(socket_.into_c_fd(), backlog); });
  auto res = into_sys_result(listen).map([=](auto n) { return listener; });
  ASYNC_TRY(res);
  INFO("{} listening\n", socket_);

  co_return IoResult<TcpListener>::ok(listener);
}

auto net::TcpSocket::set_reuseaddr(bool reuseaddr) -> IoResult<void> {
  return into_sys_result(::setsockopt(socket_.into_c_fd(), SOL_SOCKET,
                                      SO_REUSEADDR, &reuseaddr, sizeof(bool)))
      .map([](auto n) {});
}

auto net::TcpSocket::set_reuseport(bool reuseport) -> IoResult<void> {
  return into_sys_result(::setsockopt(socket_.into_c_fd(), SOL_SOCKET,
                                      SO_REUSEADDR, &reuseport, sizeof(bool)))
      .map([](auto n) {});
}

auto net::TcpSocket::new_v4() -> IoResult<TcpSocket> {
  return into_sys_result(::socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0))
      .map([](auto fd) { return TcpSocket(fd); });
}

auto net::TcpSocket::new_v6() -> IoResult<TcpSocket> {
  return into_sys_result(::socket(AF_INET6, SOCK_STREAM | SOCK_NONBLOCK, 0))
      .map([](auto fd) { return TcpSocket(fd); });
}

net::TcpSocket::TcpSocket(int fd) : socket_(fd) {}

auto net::TcpStream::connect(SocketAddr addr) -> Future<IoResult<TcpStream>> {
  auto socket = addr.is_v4() ? TcpSocket::new_v4() : TcpSocket::new_v6();
  if (socket.is_err()) {
    co_return IoResult<TcpStream>::err(socket.unwrap_err());
  }
  co_return co_await socket.unwrap().connect(addr);
}

auto net::TcpStream::read(std::vector<char> *buf)
    -> Future<IoResult<uintptr_t>> {
  using CoOutput = IoResult<uintptr_t>;

  class Future : public runtime::Future<CoOutput> {
   public:
    Future(std::vector<char> *buf, net::TcpStream *self)
        : runtime::Future<CoOutput>(nullptr), buf_(buf), self_(self) {}

    auto poll(runtime::Handle<void> self) -> runtime::Poll<CoOutput> override {
      if (self_->event_.readable()) {
        auto n = ::read(self_->socket_.into_c_fd(), buf_->data(), buf_->size());
        if (n != -1) {
          INFO("read {} bytes from {}\n", n, self_->socket_);
          return runtime::Ready<CoOutput>{CoOutput::ok(n)};
        }
        if (errno != EAGAIN && errno != EWOULDBLOCK) {
          return runtime::Ready<CoOutput>{
              CoOutput::err(into_sys_result(-1).unwrap_err())};
        }
      }
      self_->event_.clear_readable();
      self_->event_.future_ = this;
      return runtime::Pending();
    }

   private:
    net::TcpStream *self_;
    std::vector<char> *buf_;
  };

  auto result = co_await Future(buf, this);
  event_.future_ = nullptr;
  co_return result;
}

template <typename I>
auto net::TcpStream::write(I begin, I end) -> Future<IoResult<uintptr_t>> {
  using CoOutput = IoResult<uintptr_t>;

  class Future : public runtime::Future<CoOutput> {
   public:
    Future(I begin, I end, TcpStream *self)
        : runtime::Future<CoOutput>(nullptr),
          begin_(begin),
          end_(end),
          self_(self) {}

    auto poll(runtime::Handle<void> self) -> runtime::Poll<CoOutput> override {
      if (self_->event_.writeable()) {
        auto n = ::write(self_->socket_.into_c_fd(), &*begin_, end_ - begin_);
        auto nbytes = into_sys_result(n).map(
            [](auto n) -> uintptr_t { return static_cast<uintptr_t>(n); });
        INFO("write {} bytes to {}\n", n, self_->socket_);
        return runtime::Ready<CoOutput>{nbytes};
      }
      self_->event_.clear_writeable();
      self_->event_.future_ = this;
      return runtime::Pending();
    }

   private:
    TcpStream *self_;
    I begin_;
    I end_;
  };

  auto result = co_await Future(begin, end, this);
  event_.future_ = nullptr;
  co_return result;
}

auto net::TcpStream::write(const std::vector<char> &buf)
    -> Future<IoResult<uintptr_t>> {
  co_return co_await write(buf.cbegin(), buf.cend());
}

auto net::TcpStream::write_all(const std::vector<char> &buf)
    -> Future<IoResult<void>> {
  auto len = buf.size();
  auto total_write = 0;
  auto begin = buf.cbegin();
  auto end = buf.cend();

  while (total_write != len) {
    auto res = (co_await write(begin, end)).map([&](auto n) {
      total_write += n;
      begin += n;
    });
    if (res.is_err()) {
      auto error = res.unwrap_err();
      if (error.errno_ != EAGAIN && error.errno_ != EWOULDBLOCK &&
          error.errno_ != EINTR) {
        co_return IoResult<void>::err(error);
      }
    }
  }
  co_return IoResult<void>::ok();
}

auto net::TcpStream::flush() -> Future<IoResult<void>> {
  co_return IoResult<void>::ok();
}

auto net::TcpStream::shutdown(Shutdown shutdown) const
    -> Future<IoResult<void>> {
  auto res = into_sys_result(co_await runtime::AsyncFuture<int>([&]() {
               return ::shutdown(
                   socket_.into_c_fd(),
                   static_cast<std::underlying_type_t<Shutdown>>(shutdown));
             })).map([](auto n) {});
  ASYNC_TRY(res);
  INFO("shutdown {}\n", socket_);
  co_return IoResult<void>::ok();
}

net::TcpStream::TcpStream(Socket socket, reactor::Event::State state)
    : socket_(socket), event_({.state_ = state, .fd_ = socket.into_c_fd()}) {
  auto *io_handle = runtime::RuntimeCtx::get_ctx()->io_handle();
  auto register_result =
      io_handle->register_local(event_, reactor::Interest::All);
  if (register_result.is_err()) {
    auto err = register_result.unwrap_err().errno_;
    if (err == EEXIST) {
      io_handle->reregister_local(event_, reactor::Interest::All).unwrap();
    }
  }
}

auto net::TcpListener::bind(SocketAddr addr) -> Future<IoResult<TcpListener>> {
  using runtime::Handle;
  using runtime::Ready;

  const int max_pending_connection = 128;

  auto socket_result = TcpSocket::new_v4();
  if (socket_result.is_err()) {
    co_return IoResult<TcpListener>::err(socket_result.unwrap_err());
  }
  auto tcp_socket = socket_result.unwrap();
  auto bind_result = co_await tcp_socket.bind(addr);
  if (bind_result.is_err()) {
    co_return IoResult<TcpListener>::err(bind_result.unwrap_err());
  }
  co_return co_await tcp_socket.listen(max_pending_connection);
}

auto net::TcpListener::accept()
    -> Future<IoResult<std::pair<TcpStream, SocketAddr>>> {
  using CoOutput = IoResult<std::pair<TcpStream, SocketAddr>>;

  class Future : public runtime::Future<CoOutput> {
   public:
    explicit Future(TcpListener *self)
        : runtime::Future<CoOutput>(nullptr), self_(self) {}

    auto poll(runtime::Handle<void> self) -> runtime::Poll<CoOutput> override {
      if (self_->event_.fd_ == 0) {
        self_->event_ =
            reactor::Event{.fd_ = self_->socket_.into_c_fd(), .future_ = this};
        auto res = runtime::RuntimeCtx::get_ctx()->io_handle()->Register(
            self_->event_, reactor::Interest::Read);
        if (res.is_err()) {
          return runtime::Ready<CoOutput>{CoOutput::err(res.unwrap_err())};
        }
      }

      if (self_->event_.readable()) {
        sockaddr_in addr_in{};
        socklen_t addrlen = sizeof(addr_in);
        auto accept_result = into_sys_result(
            ::accept4(self_->socket_.into_c_fd(),
                      static_cast<sockaddr *>(static_cast<void *>(&addr_in)),
                      &addrlen, SOCK_NONBLOCK));
        if (accept_result.is_err()) {
          auto err = accept_result.unwrap_err();
          if (err.errno_ == EAGAIN || err.errno_ == EWOULDBLOCK) {
            self_->event_.state_ = reactor::Event::State::Pending;
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
        INFO("accept from {} new connect={{{}, addr:{}}}\n", self_->socket_,
             socket, sock_addr);
        return runtime::Ready<CoOutput>{CoOutput::ok(
            TcpStream(socket, reactor::Event::State::Writable), sock_addr)};
      }
      return runtime::Pending();
    }

   private:
    TcpListener *self_;
  };

  auto res = co_await Future(this);
  co_return res;
}

net::TcpListener::TcpListener(int fd) : socket_(fd) {}

template <typename FormatContext>
auto fmt::formatter<net::TcpSocket>::format(const net::TcpSocket &tcp_socket,
                                            FormatContext &ctx) const
    -> decltype(ctx.out()) {
  return format_to(ctx.out(), "TcpSocket{{socket_={}}}", tcp_socket.socket_);
}

template <typename FormatContext>
auto fmt::formatter<net::TcpStream>::format(const net::TcpStream &tcp_stream,
                                            FormatContext &ctx) const
    -> decltype(ctx.out()) {
  return format_to(ctx.out(), "TcpStream{{socket_={}}}", tcp_stream.socket_);
}

template <typename FormatContext>
auto fmt::formatter<net::TcpListener>::format(
    const net::TcpListener &tcp_listener, FormatContext &ctx) const
    -> decltype(ctx.out()) {
  return format_to(ctx.out(), "TcpListener{{socket_={}}}",
                   tcp_listener.socket_);
}

template auto fmt::formatter<net::TcpSocket>::format(
    const net::TcpSocket &tcp_socket,
    fmt::basic_format_context<fmt::appender, char> &ctx) const
    -> decltype(ctx.out());

template auto fmt::formatter<net::TcpStream>::format(
    const net::TcpStream &tcp_stream,
    fmt::basic_format_context<fmt::appender, char> &ctx) const
    -> decltype(ctx.out());

template auto fmt::formatter<net::TcpListener>::format(
    const net::TcpListener &tcp_listener,
    fmt::basic_format_context<fmt::appender, char> &ctx) const
    -> decltype(ctx.out());