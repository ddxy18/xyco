#include "listener.h"

#include <arpa/inet.h>

#include <cerrno>
#include <clocale>
#include <cstdint>
#include <variant>

#include "io/epoll/extra.h"
#include "runtime/async_future.h"
#include "utils/error.h"

template <typename T>
using Future = xyco::runtime::Future<T>;

auto xyco::net::epoll::TcpSocket::bind(SocketAddr addr)
    -> Future<utils::Result<void>> {
  auto bind_result = co_await runtime::AsyncFuture<utils::Result<int>>([&]() {
    return utils::into_sys_result(
        ::bind(socket_.into_c_fd(), addr.into_c_addr(), sizeof(sockaddr)));
  });
  if (bind_result.is_ok()) {
    INFO("{} bind to {}", socket_, addr);
  }

  co_return bind_result;
}

auto xyco::net::epoll::TcpSocket::connect(SocketAddr addr)
    -> Future<utils::Result<TcpStream>> {
  using CoOutput = utils::Result<TcpStream>;

  class Future : public runtime::Future<CoOutput> {
   public:
    explicit Future(SocketAddr addr, gsl::not_null<Socket *> socket)
        : runtime::Future<CoOutput>(nullptr),
          socket_(socket),
          addr_(addr),
          event_(std::make_shared<runtime::Event>(
              runtime::Event{.future_ = this,
                             .extra_ = std::make_unique<io::epoll::IoExtra>(
                                 io::epoll::IoExtra::Interest::Write,
                                 socket_->into_c_fd())})) {
      runtime::RuntimeCtx::get_ctx()->driver().Register<io::epoll::IoRegistry>(
          event_);
    }

    auto poll(runtime::Handle<void> self) -> runtime::Poll<CoOutput> override {
      auto *extra = dynamic_cast<io::epoll::IoExtra *>(event_->extra_.get());
      if (extra->state_.get_field<io::epoll::IoExtra::State::Error>() ||
          extra->state_.get_field<io::epoll::IoExtra::State::Writable>()) {
        int ret = -1;
        socklen_t len = sizeof(decltype(ret));
        getsockopt(socket_->into_c_fd(), SOL_SOCKET, SO_ERROR, &ret, &len);
        if (ret != 0) {
          runtime::RuntimeCtx::get_ctx()
              ->driver()
              .deregister<io::epoll::IoRegistry>(event_);
          return runtime::Ready<CoOutput>{CoOutput::err(
              utils::Error{ret, strerror_l(ret, uselocale(nullptr))})};
        }
        INFO("{} connect to {}", *socket_, addr_);
        runtime::RuntimeCtx::get_ctx()
            ->driver()
            .deregister<io::epoll::IoRegistry>(event_);
        return runtime::Ready<CoOutput>{CoOutput::ok(TcpStream(
            std::move(*socket_),
            extra->state_.get_field<io::epoll::IoExtra::State::Writable>(),
            extra->state_.get_field<io::epoll::IoExtra::State::Readable>()))};
      }
      event_->future_ = this;
      runtime::RuntimeCtx::get_ctx()
          ->driver()
          .reregister<io::epoll::IoRegistry>(event_);
      return runtime::Pending();
    }

   private:
    gsl::not_null<Socket *> socket_;
    SocketAddr addr_;
    std::shared_ptr<runtime::Event> event_;
  };

  auto connect_result =
      co_await runtime::AsyncFuture<utils::Result<int>>([&]() {
        return utils::into_sys_result(::connect(
            socket_.into_c_fd(), addr.into_c_addr(), sizeof(sockaddr)));
      });
  if (connect_result.is_ok()) {
    co_return CoOutput::ok(TcpStream(std::move(socket_), true));
  }
  auto err = connect_result.unwrap_err().errno_;
  if (err != EINPROGRESS && err != EAGAIN) {
    WARN("{} connect fail{{errno={}}}", socket_, errno);
    co_return CoOutput::err(utils::Error{.errno_ = err});
  }
  co_return co_await Future(addr, &socket_);
}

auto xyco::net::epoll::TcpSocket::listen(int backlog)
    -> Future<utils::Result<TcpListener>> {
  auto listen_result = co_await runtime::AsyncFuture<utils::Result<int>>([&]() {
    return utils::into_sys_result(::listen(socket_.into_c_fd(), backlog));
  });
  ASYNC_TRY(listen_result.map([&](auto n) { return TcpListener(Socket(-1)); }));
  INFO("{} listening", socket_);

  co_return utils::Result<TcpListener>::ok(TcpListener(std::move(socket_)));
}

auto xyco::net::epoll::TcpSocket::set_reuseaddr(bool reuseaddr)
    -> utils::Result<void> {
  int optval = static_cast<int>(reuseaddr);
  return utils::into_sys_result(::setsockopt(
      socket_.into_c_fd(), SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval)));
}

auto xyco::net::epoll::TcpSocket::set_reuseport(bool reuseport)
    -> utils::Result<void> {
  int optval = static_cast<int>(reuseport);
  return utils::into_sys_result(::setsockopt(
      socket_.into_c_fd(), SOL_SOCKET, SO_REUSEPORT, &optval, sizeof(optval)));
}

auto xyco::net::epoll::TcpSocket::new_v4() -> utils::Result<TcpSocket> {
  return utils::into_sys_result(
             ::socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0))
      .map([](auto file_descriptor) { return TcpSocket(file_descriptor); });
}

auto xyco::net::epoll::TcpSocket::new_v6() -> utils::Result<TcpSocket> {
  return utils::into_sys_result(
             ::socket(AF_INET6, SOCK_STREAM | SOCK_NONBLOCK, 0))
      .map([](auto file_descriptor) { return TcpSocket(file_descriptor); });
}

xyco::net::epoll::TcpSocket::TcpSocket(Socket &&socket)
    : socket_(std::move(socket)) {}

auto xyco::net::epoll::TcpStream::connect(SocketAddr addr)
    -> Future<utils::Result<TcpStream>> {
  auto socket = addr.is_v4() ? TcpSocket::new_v4() : TcpSocket::new_v6();
  if (socket.is_err()) {
    co_return utils::Result<TcpStream>::err(socket.unwrap_err());
  }
  co_return co_await socket.unwrap().connect(addr);
}

// NOLINTNEXTLINE(readability-convert-member-functions-to-static)
auto xyco::net::epoll::TcpStream::flush() -> Future<utils::Result<void>> {
  co_return utils::Result<void>::ok();
}

auto xyco::net::epoll::TcpStream::shutdown(io::Shutdown shutdown) const
    -> Future<utils::Result<void>> {
  ASYNC_TRY(co_await runtime::AsyncFuture<utils::Result<int>>([&]() {
    return utils::into_sys_result(::shutdown(
        socket_.into_c_fd(),
        static_cast<std::underlying_type_t<io::Shutdown>>(shutdown)));
  }));
  INFO("shutdown {}", socket_);

  co_return utils::Result<void>::ok();
}

xyco::net::epoll::TcpStream::~TcpStream() {
  if (socket_.into_c_fd() != -1 &&
      dynamic_cast<io::epoll::IoExtra *>(event_->extra_.get())
          ->state_.get_field<io::epoll::IoExtra::State::Registered>()) {
    runtime::RuntimeCtx::get_ctx()->driver().deregister<io::epoll::IoRegistry>(
        event_);
  }
}

xyco::net::epoll::TcpStream::TcpStream(Socket &&socket, bool writable,
                                       bool readable)
    : socket_(std::move(socket)),
      event_(std::make_shared<runtime::Event>(runtime::Event{
          .extra_ = std::make_unique<io::epoll::IoExtra>(
              io::epoll::IoExtra::Interest::All, socket_.into_c_fd())})) {
  auto &state =
      dynamic_cast<io::epoll::IoExtra *>(event_->extra_.get())->state_;
  if (writable) {
    state.set_field<io::epoll::IoExtra::State::Writable>();
  }
  if (readable) {
    state.set_field<io::epoll::IoExtra::State::Readable>();
  }
}

auto xyco::net::epoll::TcpListener::bind(SocketAddr addr)
    -> Future<utils::Result<TcpListener>> {
  using runtime::Handle;
  using runtime::Ready;

  const int max_pending_connection = 128;

  auto socket_result = TcpSocket::new_v4();
  if (socket_result.is_err()) {
    co_return utils::Result<TcpListener>::err(socket_result.unwrap_err());
  }
  auto tcp_socket = socket_result.unwrap();
  auto bind_result = co_await tcp_socket.bind(addr);
  if (bind_result.is_err()) {
    co_return utils::Result<TcpListener>::err(bind_result.unwrap_err());
  }
  co_return co_await tcp_socket.listen(max_pending_connection);
}

auto xyco::net::epoll::TcpListener::accept()
    -> Future<utils::Result<std::pair<TcpStream, SocketAddr>>> {
  using CoOutput = utils::Result<std::pair<TcpStream, SocketAddr>>;

  class Future : public runtime::Future<CoOutput> {
   public:
    auto poll(runtime::Handle<void> self) -> runtime::Poll<CoOutput> override {
      auto *extra =
          dynamic_cast<io::epoll::IoExtra *>(self_->event_->extra_.get());
      if (!extra->state_.get_field<io::epoll::IoExtra::State::Registered>()) {
        self_->event_->future_ = this;
        runtime::RuntimeCtx::get_ctx()
            ->driver()
            .Register<io::epoll::IoRegistry>(self_->event_);
        TRACE("register accept {}", *self_->event_);
        return runtime::Pending();
      }
      if (extra->state_.get_field<io::epoll::IoExtra::State::Error>()) {
        return runtime::Ready<CoOutput>{CoOutput::err(
            utils::Error{.errno_ = 1, .info_ = "epoll state error"})};
      }
      if (extra->state_.get_field<io::epoll::IoExtra::State::Readable>()) {
        auto accept_result = utils::into_sys_result(
            ::accept4(self_->socket_.into_c_fd(),
                      static_cast<sockaddr *>(static_cast<void *>(&addr_in_)),
                      &addrlen_, SOCK_NONBLOCK));
        if (accept_result.is_err()) {
          auto err = accept_result.unwrap_err();
          if (err.errno_ == EAGAIN || err.errno_ == EWOULDBLOCK) {
            self_->event_->future_ = this;
            TRACE("reregister accept {}", *self_->event_);
            runtime::RuntimeCtx::get_ctx()
                ->driver()
                .reregister<io::epoll::IoRegistry>(self_->event_);
            return runtime::Pending();
          }
          return runtime::Ready<CoOutput>{CoOutput::err(err)};
        }
        std::string ip_addr(INET_ADDRSTRLEN, 0);
        auto sock_addr = SocketAddr::new_v4(
            Ipv4Addr(::inet_ntop(addr_in_.sin_family, &addr_in_.sin_addr,
                                 ip_addr.data(), ip_addr.size())),
            addr_in_.sin_port);
        auto socket = Socket(accept_result.unwrap());
        INFO("accept from {} new connect={{{}, addr:{}}}", self_->socket_,
             socket, sock_addr);
        return runtime::Ready<CoOutput>{
            CoOutput::ok(TcpStream(std::move(socket)), sock_addr)};
      }
      return runtime::Pending();
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
    sockaddr_in addr_in_{};
    socklen_t addrlen_{sizeof(addr_in_)};
  };

  co_return co_await Future(this);
}

xyco::net::epoll::TcpListener::~TcpListener() {
  if (socket_.into_c_fd() != -1 && event_ != nullptr &&
      dynamic_cast<io::epoll::IoExtra *>(event_->extra_.get())
          ->state_.get_field<io::epoll::IoExtra::State::Registered>()) {
    runtime::RuntimeCtx::get_ctx()->driver().deregister<io::epoll::IoRegistry>(
        event_);
  }
}

xyco::net::epoll::TcpListener::TcpListener(Socket &&socket)
    : socket_(std::move(socket)),
      event_(std::make_shared<runtime::Event>(runtime::Event{
          .extra_ = std::make_unique<io::epoll::IoExtra>(
              io::epoll::IoExtra::Interest::Read, socket_.into_c_fd())})) {}
