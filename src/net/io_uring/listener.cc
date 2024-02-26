module;

#include <coroutine>
#include <expected>
#include <gsl/pointers>
#include <variant>

#include "xyco/utils/result.h"

module xyco.net.uring;

import xyco.logging;
import xyco.task;
import xyco.libc;

template <typename T>
using Future = xyco::runtime::Future<T>;

auto xyco::net::uring::TcpSocket::bind(SocketAddr addr)
    -> Future<utils::Result<void>> {
  auto bind_result = co_await task::BlockingTask([&]() {
    return utils::into_sys_result(xyco::libc::bind(
        socket_.into_c_fd(), addr.into_c_addr(), sizeof(xyco::libc::sockaddr)));
  });
  if (bind_result) {
    logging::info("{} bind to {}", socket_, addr);
  }

  co_return bind_result.transform([]([[maybe_unused]] auto n) {});
}

auto xyco::net::uring::TcpSocket::connect(SocketAddr addr)
    -> Future<utils::Result<TcpStream>> {
  using CoOutput = utils::Result<TcpStream>;

  // NOLINTNEXTLINE(cppcoreguidelines-avoid-reference-coroutine-parameters)
  class Future : public runtime::Future<CoOutput> {
   public:
    explicit Future(SocketAddr addr, gsl::not_null<Socket *> socket)
        : runtime::Future<CoOutput>(nullptr),
          socket_(socket),
          addr_(addr),
          event_(std::make_shared<runtime::Event>(runtime::Event{
              .extra_ = std::make_unique<io::uring::IoExtra>()})) {}

    auto poll([[maybe_unused]] runtime::Handle<void> self)
        -> runtime::Poll<CoOutput> override {
      auto *extra = dynamic_cast<io::uring::IoExtra *>(event_->extra_.get());
      if (!extra->state_.get_field<io::uring::IoExtra::State::Completed>()) {
        event_->future_ = this;
        extra->args_ = io::uring::IoExtra::Connect{
            .addr_ = addr_.into_c_addr(),
            .addrlen_ = sizeof(xyco::libc::sockaddr)};
        extra->fd_ = socket_->into_c_fd();
        runtime::RuntimeCtx::get_ctx()
            ->driver()
            .Register<io::uring::IoRegistry>(event_);

        return runtime::Pending();
      }

      extra->state_.set_field<io::uring::IoExtra::State::Completed, false>();
      if (extra->return_ < 0) {
        return runtime::Ready<CoOutput>{
            std::unexpected(utils::Error{.errno_ = -extra->return_})};
      }
      logging::info("{} connect to {}", *socket_, addr_);
      return runtime::Ready<CoOutput>{TcpStream(std::move(*socket_))};
    }

   private:
    gsl::not_null<Socket *> socket_;
    SocketAddr addr_;
    std::shared_ptr<runtime::Event> event_;
  };

  co_return co_await Future(addr, &socket_);
}

auto xyco::net::uring::TcpSocket::listen(int backlog)
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

auto xyco::net::uring::TcpSocket::set_reuseaddr(bool reuseaddr)
    -> utils::Result<void> {
  int optval = static_cast<int>(reuseaddr);
  return utils::into_sys_result(
             xyco::libc::setsockopt(
                 socket_.into_c_fd(), xyco::libc::K_SOL_SOCKET,
                 xyco::libc::K_SO_REUSEADDR, &optval, sizeof(optval)))
      .transform([]([[maybe_unused]] auto result) {});
}

auto xyco::net::uring::TcpSocket::set_reuseport(bool reuseport)
    -> utils::Result<void> {
  int optval = static_cast<int>(reuseport);
  return utils::into_sys_result(
             xyco::libc::setsockopt(
                 socket_.into_c_fd(), xyco::libc::K_SOL_SOCKET,
                 xyco::libc::K_SO_REUSEPORT, &optval, sizeof(optval)))
      .transform([]([[maybe_unused]] auto result) {});
}

auto xyco::net::uring::TcpSocket::new_v4() -> utils::Result<TcpSocket> {
  return utils::into_sys_result(xyco::libc::socket(xyco::libc::K_AF_INET,
                                                   xyco::libc::K_SOCK_STREAM,
                                                   0))
      .transform(
          [](auto file_descriptor) { return TcpSocket(file_descriptor); });
}

auto xyco::net::uring::TcpSocket::new_v6() -> utils::Result<TcpSocket> {
  return utils::into_sys_result(xyco::libc::socket(xyco::libc::K_AF_INET6,
                                                   xyco::libc::K_SOCK_STREAM,
                                                   0))
      .transform(
          [](auto file_descriptor) { return TcpSocket(file_descriptor); });
}

xyco::net::uring::TcpSocket::TcpSocket(Socket &&socket)
    : socket_(std::move(socket)) {}

auto xyco::net::uring::TcpStream::connect(SocketAddr addr)
    -> Future<utils::Result<TcpStream>> {
  auto socket = addr.is_v4() ? TcpSocket::new_v4() : TcpSocket::new_v6();
  if (!socket) {
    co_return std::unexpected(socket.error());
  }
  co_return co_await socket->connect(addr);
}

// NOLINTNEXTLINE(readability-convert-member-functions-to-static)
auto xyco::net::uring::TcpStream::flush() -> Future<utils::Result<void>> {
  co_return {};
}

auto xyco::net::uring::TcpStream::shutdown(io::Shutdown shutdown)
    -> Future<utils::Result<void>> {
  using CoOutput = utils::Result<void>;

  // NOLINTNEXTLINE(cppcoreguidelines-avoid-reference-coroutine-parameters)
  class Future : public runtime::Future<CoOutput> {
   public:
    explicit Future(io::Shutdown shutdown, TcpStream *tcp_stream)
        : runtime::Future<CoOutput>(nullptr),
          self_(tcp_stream),
          shutdown_(shutdown) {}

    auto poll([[maybe_unused]] runtime::Handle<void> self)
        -> runtime::Poll<CoOutput> override {
      auto *extra =
          dynamic_cast<io::uring::IoExtra *>(self_->event_->extra_.get());
      if (!extra->state_.get_field<io::uring::IoExtra::State::Completed>()) {
        self_->event_->future_ = this,
        extra->args_ = io::uring::IoExtra::Shutdown{.shutdown_ = shutdown_};
        extra->fd_ = self_->socket_.into_c_fd();
        runtime::RuntimeCtx::get_ctx()
            ->driver()
            .Register<io::uring::IoRegistry>(self_->event_);

        return runtime::Pending();
      }

      extra->state_.set_field<io::uring::IoExtra::State::Completed, false>();
      if (extra->return_ < 0) {
        return runtime::Ready<CoOutput>{
            std::unexpected(utils::Error{.errno_ = -extra->return_})};
      }
      logging::info("shutdown {}", self_->socket_);
      return runtime::Ready<CoOutput>{{}};
    }

   private:
    TcpStream *self_;
    io::Shutdown shutdown_;
  };

  auto result = co_await Future(shutdown, this);
  event_->future_ = nullptr;
  co_return result;
}

xyco::net::uring::TcpStream::TcpStream(Socket &&socket)
    : socket_(std::move(socket)),
      event_(std::make_shared<runtime::Event>(
          runtime::Event{.extra_ = std::make_unique<io::uring::IoExtra>()})) {
  auto *extra = dynamic_cast<io::uring::IoExtra *>(event_->extra_.get());
  extra->fd_ = socket_.into_c_fd();
}

auto xyco::net::uring::TcpListener::bind(SocketAddr addr)
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

auto xyco::net::uring::TcpListener::accept()
    -> Future<utils::Result<std::pair<TcpStream, SocketAddr>>> {
  using CoOutput = utils::Result<std::pair<TcpStream, SocketAddr>>;

  class Future : public runtime::Future<CoOutput> {
   public:
    auto poll([[maybe_unused]] runtime::Handle<void> self)
        -> runtime::Poll<CoOutput> override {
      auto *extra =
          dynamic_cast<io::uring::IoExtra *>(self_->event_->extra_.get());
      if (!extra->state_.get_field<io::uring::IoExtra::State::Completed>()) {
        self_->event_->future_ = this;
        extra->args_ = io::uring::IoExtra::Accept{
            .addr_ = static_cast<xyco::libc::sockaddr *>(
                static_cast<void *>(&addr_)),
            .addrlen_ = &addr_len_};
        extra->fd_ = self_->socket_.into_c_fd();
        runtime::RuntimeCtx::get_ctx()
            ->driver()
            .Register<io::uring::IoRegistry>(self_->event_);

        return runtime::Pending();
      }

      extra->state_.set_field<io::uring::IoExtra::State::Completed, false>();
      if (extra->return_ < 0) {
        return runtime::Ready<CoOutput>{
            std::unexpected(utils::Error{.errno_ = -extra->return_})};
      }
      std::string ip_addr(xyco::libc::K_INET_ADDRSTRLEN, 0);
      auto sock_addr = SocketAddr::new_v4(
          Ipv4Addr(xyco::libc::inet_ntop(addr_.sin_family, &addr_.sin_addr,
                                         ip_addr.data(), ip_addr.size())),
          addr_.sin_port);
      auto socket = Socket(extra->return_);
      logging::info("accept from {} new connect={{{}, addr:{}}}",
                    self_->socket_, socket, sock_addr);
      return runtime::Ready<CoOutput>{
          std::pair{TcpStream(std::move(socket)), sock_addr}};
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
    xyco::libc::sockaddr_in addr_{};
    xyco::libc::socklen_t addr_len_{sizeof(addr_)};
  };

  co_return co_await Future(this);
}

xyco::net::uring::TcpListener::TcpListener(Socket &&socket)
    : socket_(std::move(socket)),
      event_(std::make_shared<runtime::Event>(
          runtime::Event{.extra_ = std::make_unique<io::uring::IoExtra>()})) {}
