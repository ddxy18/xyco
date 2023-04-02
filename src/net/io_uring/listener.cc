#include "listener.h"

#include <arpa/inet.h>

#include <cerrno>
#include <clocale>
#include <cstdint>
#include <variant>

#include "runtime/async_future.h"

template <typename T>
using Future = xyco::runtime::Future<T>;

auto xyco::net::uring::TcpSocket::bind(SocketAddr addr)
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

    auto poll(runtime::Handle<void> self) -> runtime::Poll<CoOutput> override {
      auto *extra = dynamic_cast<io::uring::IoExtra *>(event_->extra_.get());
      if (!extra->state_.get_field<io::uring::IoExtra::State::Completed>()) {
        event_->future_ = this;
        extra->args_ = io::uring::IoExtra::Connect{
            .addr_ = addr_.into_c_addr(), .addrlen_ = sizeof(sockaddr)};
        extra->fd_ = socket_->into_c_fd();
        runtime::RuntimeCtx::get_ctx()
            ->driver()
            .Register<io::uring::IoRegistry>(event_);

        return runtime::Pending();
      }

      extra->state_.set_field<io::uring::IoExtra::State::Completed, false>();
      if (extra->return_ < 0) {
        return runtime::Ready<CoOutput>{
            CoOutput::err(utils::Error{.errno_ = -extra->return_})};
      }
      INFO("{} connect to {}", *socket_, addr_);
      return runtime::Ready<CoOutput>{
          CoOutput::ok(TcpStream(std::move(*socket_)))};
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
  auto listen_result = co_await runtime::AsyncFuture<utils::Result<int>>([&]() {
    return utils::into_sys_result(::listen(socket_.into_c_fd(), backlog));
  });
  ASYNC_TRY(listen_result.map([&](auto n) { return TcpListener(Socket(-1)); }));
  INFO("{} listening", socket_);

  co_return utils::Result<TcpListener>::ok(TcpListener(std::move(socket_)));
}

auto xyco::net::uring::TcpSocket::set_reuseaddr(bool reuseaddr)
    -> utils::Result<void> {
  int optval = static_cast<int>(reuseaddr);
  return utils::into_sys_result(::setsockopt(
      socket_.into_c_fd(), SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval)));
}

auto xyco::net::uring::TcpSocket::set_reuseport(bool reuseport)
    -> utils::Result<void> {
  int optval = static_cast<int>(reuseport);
  return utils::into_sys_result(::setsockopt(
      socket_.into_c_fd(), SOL_SOCKET, SO_REUSEPORT, &optval, sizeof(optval)));
}

auto xyco::net::uring::TcpSocket::new_v4() -> utils::Result<TcpSocket> {
  return utils::into_sys_result(::socket(AF_INET, SOCK_STREAM, 0))
      .map([](auto file_descriptor) { return TcpSocket(file_descriptor); });
}

auto xyco::net::uring::TcpSocket::new_v6() -> utils::Result<TcpSocket> {
  return utils::into_sys_result(::socket(AF_INET6, SOCK_STREAM, 0))
      .map([](auto file_descriptor) { return TcpSocket(file_descriptor); });
}

xyco::net::uring::TcpSocket::TcpSocket(Socket &&socket)
    : socket_(std::move(socket)) {}

auto xyco::net::uring::TcpStream::connect(SocketAddr addr)
    -> Future<utils::Result<TcpStream>> {
  auto socket = addr.is_v4() ? TcpSocket::new_v4() : TcpSocket::new_v6();
  if (socket.is_err()) {
    co_return utils::Result<TcpStream>::err(socket.unwrap_err());
  }
  co_return co_await socket.unwrap().connect(addr);
}

auto xyco::net::uring::TcpStream::flush() -> Future<utils::Result<void>> {
  co_return utils::Result<void>::ok();
}

auto xyco::net::uring::TcpStream::shutdown(io::Shutdown shutdown)
    -> Future<utils::Result<void>> {
  using CoOutput = utils::Result<void>;

  // NOLINTNEXTLINE(cppcoreguidelines-avoid-reference-coroutine-parameters)
  class Future : public runtime::Future<CoOutput> {
   public:
    explicit Future(io::Shutdown shutdown, TcpStream *tcp_stream)
        : runtime::Future<CoOutput>(nullptr),
          shutdown_(shutdown),
          self_(tcp_stream) {}

    auto poll(runtime::Handle<void> self) -> runtime::Poll<CoOutput> override {
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
            CoOutput::err(utils::Error{.errno_ = -extra->return_})};
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
  co_return result;
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

auto xyco::net::uring::TcpListener::accept()
    -> Future<utils::Result<std::pair<TcpStream, SocketAddr>>> {
  using CoOutput = utils::Result<std::pair<TcpStream, SocketAddr>>;

  class Future : public runtime::Future<CoOutput> {
   public:
    auto poll(runtime::Handle<void> self) -> runtime::Poll<CoOutput> override {
      auto *extra =
          dynamic_cast<io::uring::IoExtra *>(self_->event_->extra_.get());
      if (!extra->state_.get_field<io::uring::IoExtra::State::Completed>()) {
        self_->event_->future_ = this;
        extra->args_ = io::uring::IoExtra::Accept{
            .addr_ = static_cast<sockaddr *>(static_cast<void *>(&addr_)),
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
            CoOutput::err(utils::Error{.errno_ = -extra->return_})};
      }
      std::string ip_addr(INET_ADDRSTRLEN, 0);
      auto sock_addr = SocketAddr::new_v4(
          Ipv4Addr(::inet_ntop(addr_.sin_family, &addr_.sin_addr,
                               ip_addr.data(), ip_addr.size())),
          addr_.sin_port);
      auto socket = Socket(extra->return_);
      INFO("accept from {} new connect={{{}, addr:{}}}", self_->socket_, socket,
           sock_addr);
      return runtime::Ready<CoOutput>{
          CoOutput::ok(TcpStream(std::move(socket)), sock_addr)};
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
    sockaddr_in addr_{};
    socklen_t addr_len_{sizeof(addr_)};
  };

  co_return co_await Future(this);
}

xyco::net::uring::TcpListener::TcpListener(Socket &&socket)
    : socket_(std::move(socket)),
      event_(std::make_shared<runtime::Event>(
          runtime::Event{.extra_ = std::make_unique<io::uring::IoExtra>()})) {}
