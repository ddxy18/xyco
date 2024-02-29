module;

#include <cerrno>
#include <clocale>
#include <coroutine>
#include <expected>
#include <gsl/pointers>

#include "xyco/utils/result.h"

module xyco.net.epoll;

import xyco.logging;
import xyco.error;
import xyco.task;
import xyco.io;
import xyco.net.common;
import xyco.libc;

template <typename T>
using Future = xyco::runtime::Future<T>;

auto xyco::net::epoll::TcpSocket::bind(SocketAddr addr)
    -> Future<utils::Result<void>> {
  auto bind_result = co_await task::BlockingTask([&]() {
    return utils::into_sys_result(xyco::libc::bind(
        socket_.into_c_fd(), addr.into_c_addr(), sizeof(xyco::libc::sockaddr)));
  });
  if (bind_result) {
    logging::info("{} bind to {}", socket_, addr);
  }
  co_return bind_result.transform([]([[maybe_unused]] auto bind_result) {});
}

auto xyco::net::epoll::TcpSocket::connect(SocketAddr addr)
    -> Future<utils::Result<TcpStream>> {
  using CoOutput = utils::Result<TcpStream>;

  // NOLINTNEXTLINE(cppcoreguidelines-avoid-reference-coroutine-parameters)
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
                                 socket_->into_c_fd())})) {}

    auto poll([[maybe_unused]] runtime::Handle<void> self)
        -> runtime::Poll<CoOutput> override {
      auto *extra = dynamic_cast<io::epoll::IoExtra *>(event_->extra_.get());
      if (extra->state_.get_field<io::epoll::IoExtra::State::Error>() ||
          extra->state_.get_field<io::epoll::IoExtra::State::Writable>()) {
        int ret = -1;
        xyco::libc::socklen_t len = sizeof(decltype(ret));
        xyco::libc::getsockopt(socket_->into_c_fd(), xyco::libc::K_SOL_SOCKET,
                               xyco::libc::K_SO_ERROR, &ret, &len);
        if (ret != 0) {
          runtime::RuntimeCtx::get_ctx()
              ->driver()
              .deregister<io::epoll::IoRegistry>(event_);
          return runtime::Ready<CoOutput>{std::unexpected(utils::Error{
              .errno_ = ret, .info_ = strerror_l(ret, ::uselocale(nullptr))})};
        }
        logging::info("{} connect to {}", *socket_, addr_);
        runtime::RuntimeCtx::get_ctx()
            ->driver()
            .deregister<io::epoll::IoRegistry>(event_);
        return runtime::Ready<CoOutput>{TcpStream(
            std::move(*socket_),
            extra->state_.get_field<io::epoll::IoExtra::State::Writable>(),
            extra->state_.get_field<io::epoll::IoExtra::State::Readable>())};
      }
      event_->future_ = this;
      runtime::RuntimeCtx::get_ctx()->driver().Register<io::epoll::IoRegistry>(
          event_);
      return runtime::Pending();
    }

   private:
    gsl::not_null<Socket *> socket_;
    SocketAddr addr_;
    std::shared_ptr<runtime::Event> event_;
  };

  auto connect_result = co_await task::BlockingTask([&]() {
    return utils::into_sys_result(xyco::libc::connect(
        socket_.into_c_fd(), addr.into_c_addr(), sizeof(xyco::libc::sockaddr)));
  });
  if (connect_result) {
    co_return TcpStream(std::move(socket_), true);
  }
  auto err = connect_result.error().errno_;
  if (err != EINPROGRESS && err != EAGAIN) {
    logging::warn("{} connect fail{{errno={}}}", socket_, errno);
    co_return std::unexpected(utils::Error{.errno_ = err, .info_ = ""});
  }
  co_return co_await Future(addr, &socket_);
}

auto xyco::net::epoll::TcpSocket::listen(int backlog)
    -> Future<utils::Result<TcpListener>> {
  auto listen_result = co_await task::BlockingTask([&]() {
    return utils::into_sys_result(
        xyco::libc::listen(socket_.into_c_fd(), backlog));
  });
  ASYNC_TRY(listen_result.transform(
      [&]([[maybe_unused]] auto n) { return TcpListener(Socket(-1)); }));
  logging::info("{} listening", socket_);

  co_return TcpListener(std::move(socket_));
}

auto xyco::net::epoll::TcpSocket::set_reuseaddr(bool reuseaddr)
    -> utils::Result<void> {
  int optval = static_cast<int>(reuseaddr);
  return utils::into_sys_result(
             xyco::libc::setsockopt(
                 socket_.into_c_fd(), xyco::libc::K_SOL_SOCKET,
                 xyco::libc::K_SO_REUSEADDR, &optval, sizeof(optval)))
      .transform([]([[maybe_unused]] auto result) {});
}

auto xyco::net::epoll::TcpSocket::set_reuseport(bool reuseport)
    -> utils::Result<void> {
  int optval = static_cast<int>(reuseport);
  return utils::into_sys_result(
             xyco::libc::setsockopt(
                 socket_.into_c_fd(), xyco::libc::K_SOL_SOCKET,
                 xyco::libc::K_SO_REUSEPORT, &optval, sizeof(optval)))
      .transform([]([[maybe_unused]] auto result) {});
}

auto xyco::net::epoll::TcpSocket::new_v4() -> utils::Result<TcpSocket> {
  return utils::into_sys_result(xyco::libc::socket(xyco::libc::K_AF_INET,
                                                   SOCK_STREAM | SOCK_NONBLOCK,
                                                   0))
      .transform(
          [](auto file_descriptor) { return TcpSocket(file_descriptor); });
}

auto xyco::net::epoll::TcpSocket::new_v6() -> utils::Result<TcpSocket> {
  return utils::into_sys_result(xyco::libc::socket(xyco::libc::K_AF_INET6,
                                                   SOCK_STREAM | SOCK_NONBLOCK,
                                                   0))
      .transform(
          [](auto file_descriptor) { return TcpSocket(file_descriptor); });
}

xyco::net::epoll::TcpSocket::TcpSocket(Socket &&socket)
    : socket_(std::move(socket)) {}

auto xyco::net::epoll::TcpStream::connect(SocketAddr addr)
    -> Future<utils::Result<TcpStream>> {
  auto socket = addr.is_v4() ? TcpSocket::new_v4() : TcpSocket::new_v6();
  if (!socket) {
    co_return std::unexpected(socket.error());
  }
  co_return co_await socket->connect(addr);
}

// NOLINTNEXTLINE(readability-convert-member-functions-to-static)
auto xyco::net::epoll::TcpStream::flush() -> Future<utils::Result<void>> {
  co_return {};
}

auto xyco::net::epoll::TcpStream::shutdown(io::Shutdown shutdown) const
    -> Future<utils::Result<void>> {
  ASYNC_TRY((co_await task::BlockingTask([&]() {
              return utils::into_sys_result(xyco::libc::shutdown(
                  socket_.into_c_fd(),
                  static_cast<std::underlying_type_t<io::Shutdown>>(shutdown)));
            })).transform([]([[maybe_unused]] auto result) {}));
  logging::info("shutdown {}", socket_);

  co_return {};
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
  if (!socket_result) {
    co_return std::unexpected(socket_result.error());
  }
  auto tcp_socket = *std::move(socket_result);
  auto bind_result = co_await tcp_socket.bind(addr);
  if (!bind_result) {
    co_return std::unexpected(bind_result.error());
  }
  co_return co_await tcp_socket.listen(max_pending_connection);
}

auto xyco::net::epoll::TcpListener::accept()
    -> Future<utils::Result<std::pair<TcpStream, SocketAddr>>> {
  using CoOutput = utils::Result<std::pair<TcpStream, SocketAddr>>;

  class Future : public runtime::Future<CoOutput> {
   public:
    auto poll([[maybe_unused]] runtime::Handle<void> self)
        -> runtime::Poll<CoOutput> override {
      auto *extra =
          dynamic_cast<io::epoll::IoExtra *>(self_->event_->extra_.get());
      if (!extra->state_.get_field<io::epoll::IoExtra::State::Registered>()) {
        self_->event_->future_ = this;
        runtime::RuntimeCtx::get_ctx()
            ->driver()
            .Register<io::epoll::IoRegistry>(self_->event_);
        logging::trace("register accept {}", *self_->event_);
        return runtime::Pending();
      }
      if (extra->state_.get_field<io::epoll::IoExtra::State::Error>()) {
        return runtime::Ready<CoOutput>{std::unexpected(
            utils::Error{.errno_ = 1, .info_ = "epoll state error"})};
      }
      if (extra->state_.get_field<io::epoll::IoExtra::State::Readable>()) {
        auto accept_result = utils::into_sys_result(xyco::libc::accept4(
            self_->socket_.into_c_fd(),
            // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
            reinterpret_cast<xyco::libc::sockaddr *>(&addr_in_), &addrlen_,
            xyco::libc::K_SOCK_NONBLOCK));
        if (!accept_result) {
          auto err = accept_result.error();
          if (err.errno_ == EAGAIN || err.errno_ == EWOULDBLOCK) {
            self_->event_->future_ = this;
            logging::trace("reregister accept {}", *self_->event_);
            runtime::RuntimeCtx::get_ctx()
                ->driver()
                .reregister<io::epoll::IoRegistry>(self_->event_);
            return runtime::Pending();
          }
          return runtime::Ready<CoOutput>{std::unexpected(err)};
        }
        std::string ip_addr(xyco::libc::K_INET_ADDRSTRLEN, 0);
        auto sock_addr =
            SocketAddr::new_v4(Ipv4Addr(xyco::libc::inet_ntop(
                                   addr_in_.sin_family, &addr_in_.sin_addr,
                                   ip_addr.data(), ip_addr.size())),
                               addr_in_.sin_port);
        auto socket = Socket(*accept_result);
        logging::info("accept from {} new connect={{{}, addr:{}}}",
                      self_->socket_, socket, sock_addr);
        return runtime::Ready<CoOutput>{
            std::pair{TcpStream(std::move(socket)), sock_addr}};
      }
      return runtime::Pending();
    }

    explicit Future(TcpListener *self)
        : runtime::Future<CoOutput>(nullptr), self_(self) {}

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
    TcpListener *self_;
    xyco::libc::sockaddr_in addr_in_{};
    xyco::libc::socklen_t addrlen_{sizeof(addr_in_)};
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
