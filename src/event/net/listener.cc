#include "listener.h"

#include <arpa/inet.h>
#include <unistd.h>

#include <clocale>

#include "event/runtime/async.h"

template <typename T>
using Future = runtime::Future<T>;

auto net::TcpSocket::bind(SocketAddr addr) -> Future<IoResult<Void>> {
  auto bind = co_await runtime::AsyncFuture(std::function<int()>([&]() {
    return ::bind(socket_.into_c_fd(), addr.into_c_addr(), sizeof(sockaddr));
  }));
  auto res = into_sys_result(bind).map(
      std::function<Void(int)>([=](auto n) { return Void(); }));
  ASYNC_TRY(res);
  INFO("{} bind to {}\n", socket_, addr);

  co_return res;
}

auto net::TcpSocket::connect(SocketAddr addr) -> Future<IoResult<TcpStream>> {
  using CoOutput = IoResult<TcpStream>;

  class Future : public runtime::Future<CoOutput> {
   public:
    explicit Future(SocketAddr addr, net::TcpSocket *self)
        : runtime::Future<CoOutput>(nullptr),
          ready_(false),
          sock_(self->socket_),
          addr_(addr),
          event_() {}

    [[nodiscard]] auto poll(runtime::Handle<void> self)
        -> runtime::Poll<CoOutput> override {
      if (!ready_) {
        auto c =
            ::connect(sock_.into_c_fd(), addr_.into_c_addr(), sizeof(sockaddr));
        if (c == -1) {
          if (errno == EINPROGRESS || errno == EAGAIN) {
            event_ = reactor::Event{reactor::Interest::Read, sock_.into_c_fd(),
                                    this};
            auto res = runtime::RuntimeCtx::get_ctx()
                           ->io_handle()
                           ->registry()
                           ->Register(&event_);
            if (res.is_err()) {
              return runtime::Ready<CoOutput>{
                  Err<TcpStream, IoError>(res.unwrap_err())};
            }
            ready_ = true;
            return runtime::Pending();
          }
          WARN("{} connect fail", sock_);
          return runtime::Ready<CoOutput>{
              Err<TcpStream, IoError>(into_sys_result(c).unwrap_err())};
        }
      }
      int ret = 0;
      socklen_t len = sizeof(decltype(ret));
      getsockopt(sock_.into_c_fd(), SOL_SOCKET, SO_ERROR, &ret, &len);
      if (ret != 0) {
        return runtime::Ready<CoOutput>{Err<TcpStream, IoError>(
            IoError{ret, strerror_l(ret, uselocale(nullptr))})};
      }
      INFO("{} connect to {}\n", sock_, addr_);
      return runtime::Ready<CoOutput>{Ok<TcpStream, IoError>(TcpStream{sock_})};
    }

   private:
    bool ready_;
    Socket sock_;
    SocketAddr addr_;
    reactor::Event event_;
  };

  auto res = co_await Future(addr, this);
  co_return res;
}

auto net::TcpSocket::listen(int backlog) -> Future<IoResult<TcpListener>> {
  auto listener = TcpListener(socket_.into_c_fd());

  auto listen = co_await runtime::AsyncFuture(std::function<int()>(
      [&]() { return ::listen(socket_.into_c_fd(), backlog); }));
  auto res = into_sys_result(listen).map(
      std::function<TcpListener(int)>([=](auto n) { return listener; }));
  ASYNC_TRY(res);
  INFO("{} listening\n", socket_);

  co_return Ok<TcpListener, IoError>(listener);
}

auto net::TcpSocket::set_reuseaddr(bool reuseaddr) -> IoResult<Void> {
  return into_sys_result(::setsockopt(socket_.into_c_fd(), SOL_SOCKET,
                                      SO_REUSEADDR, &reuseaddr, sizeof(bool)))
      .map(std::function<Void(int)>([=](auto n) { return Void(); }));
}

auto net::TcpSocket::set_reuseport(bool reuseport) -> IoResult<Void> {
  return into_sys_result(::setsockopt(socket_.into_c_fd(), SOL_SOCKET,
                                      SO_REUSEADDR, &reuseport, sizeof(bool)))
      .map(std::function<Void(int)>([=](auto n) { return Void(); }));
}

auto net::TcpSocket::new_v4() -> IoResult<TcpSocket> {
  return into_sys_result(::socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0))
      .map(
          std::function<TcpSocket(int)>([](auto fd) { return TcpSocket(fd); }));
}

auto net::TcpSocket::new_v6() -> IoResult<TcpSocket> {
  return into_sys_result(::socket(AF_INET6, SOCK_STREAM | SOCK_NONBLOCK, 0))
      .map(
          std::function<TcpSocket(int)>([](auto fd) { return TcpSocket(fd); }));
}

net::TcpSocket::TcpSocket(int fd) : socket_(fd) {}

auto net::TcpStream::connect(SocketAddr addr) -> Future<IoResult<TcpStream>> {
  auto socket = addr.is_v4() ? TcpSocket::new_v4() : TcpSocket::new_v6();
  if (socket.is_err()) {
    co_return Err<TcpStream, IoError>(socket.unwrap_err());
  }
  co_return co_await socket.unwrap().connect(addr);
}

auto net::TcpStream::read(std::vector<char> *buf)
    -> Future<IoResult<uintptr_t>> {
  using CoOutput = IoResult<uintptr_t>;

  class Future : public runtime::Future<CoOutput> {
   public:
    Future(std::vector<char> *buf, net::TcpStream *self)
        : runtime::Future<CoOutput>(nullptr),
          buf_(buf),
          ready_(false),
          self_(self),
          event_() {}

    [[nodiscard]] auto poll(runtime::Handle<void> self)
        -> runtime::Poll<CoOutput> override {
      if (!ready_) {
        ready_ = true;
        event_ = reactor::Event{reactor::Interest::Read,
                                self_->socket_.into_c_fd(), this};
        auto res = runtime::RuntimeCtx::get_ctx()
                       ->io_handle()
                       ->registry()
                       ->Register(&event_)
                       .map(std::function<uintptr_t(Void)>(
                           [](auto res) { return 0; }));
        if (res.is_err()) {
          if (res.unwrap_err().errno_ == EEXIST) {
            res = runtime::RuntimeCtx::get_ctx()
                      ->io_handle()
                      ->registry()
                      ->reregister(&event_)
                      .map(std::function<uintptr_t(Void)>(
                          [](auto res) { return 0; }));
          }
          if (res.is_err()) {
            return runtime::Ready<CoOutput>{res};
          }
        }
        return runtime::Pending();
      }
      auto n = ::read(self_->socket_.into_c_fd(), buf_->data(), buf_->size());
      if (n == -1 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
        auto res = runtime::RuntimeCtx::get_ctx()
                       ->io_handle()
                       ->registry()
                       ->reregister(&event_)
                       .map(std::function<uintptr_t(Void)>(
                           [](auto res) { return 0; }));
        return runtime::Pending();
      }
      auto nbytes =
          into_sys_result(static_cast<int>(n))
              .map(std::function<uintptr_t(int)>([](auto n) { return n; }));
      INFO("read {} bytes from {}\n", n, self_->socket_);
      return runtime::Ready<CoOutput>{nbytes};
    }

   private:
    net::TcpStream *self_;
    std::vector<char> *buf_;
    bool ready_;
    reactor::Event event_;
  };
  auto res = co_await Future(buf, this);
  co_return res;
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
          ready_(false),
          self_(self),
          event_() {}

    [[nodiscard]] auto poll(runtime::Handle<void> self)
        -> runtime::Poll<CoOutput> override {
      if (!ready_) {
        ready_ = true;
        event_ = reactor::Event{reactor::Interest::Write,
                                self_->socket_.into_c_fd(), this};
        auto res = runtime::RuntimeCtx::get_ctx()
                       ->io_handle()
                       ->registry()
                       ->Register(&event_)
                       .map(std::function<uintptr_t(Void)>(
                           [](auto res) { return 0; }));
        if (res.is_err()) {
          if (res.unwrap_err().errno_ == EEXIST) {
            res = runtime::RuntimeCtx::get_ctx()
                      ->io_handle()
                      ->registry()
                      ->reregister(&event_)
                      .map(std::function<uintptr_t(Void)>(
                          [](auto res) { return 0; }));
          }
          if (res.is_err()) {
            return runtime::Ready<CoOutput>{res};
          }
        }
        return runtime::Pending();
      }
      auto n = ::write(self_->socket_.into_c_fd(), &*begin_, end_ - begin_);
      auto nbytes = into_sys_result(n).map(std::function<uintptr_t(int)>(
          [](auto n) { return static_cast<uintptr_t>(n); }));
      INFO("write {} bytes to {}\n", n, self_->socket_);
      return runtime::Ready<CoOutput>{nbytes};
    }

   private:
    TcpStream *self_;
    I begin_;
    I end_;
    bool ready_;
    reactor::Event event_;
  };

  auto res = co_await Future(begin, end, this);
  co_return res;
}

auto net::TcpStream::write(const std::vector<char> &buf)
    -> Future<IoResult<uintptr_t>> {
  auto res = co_await write(buf.cbegin(), buf.cend());
  co_return res;
}

auto net::TcpStream::write_all(const std::vector<char> &buf)
    -> Future<IoResult<Void>> {
  auto len = buf.size();
  auto total_write = 0;
  auto begin = buf.cbegin();
  auto end = buf.cend();

  while (total_write != len) {
    auto res = (co_await write(begin, end))
                   .map(std::function<Void(uintptr_t)>([&](auto n) {
                     total_write += n;
                     begin += n;
                     return Void();
                   }));
    if (res.is_err()) {
      auto err = res.unwrap_err();
      if (err.errno_ == EAGAIN || err.errno_ == EWOULDBLOCK ||
          err.errno_ == EINTR) {
        continue;
      }
      co_return Err<Void, IoError>(err);
    }
  }
  co_return Ok<Void, IoError>(Void());
}

auto net::TcpStream::flush() -> Future<IoResult<Void>> {
  co_return Ok<Void, IoError>(Void());
}

auto net::TcpStream::shutdown(Shutdown shutdown) const
    -> Future<IoResult<Void>> {
  auto res =
      into_sys_result(co_await runtime::AsyncFuture(std::function<int()>([&]() {
        return ::shutdown(
            socket_.into_c_fd(),
            static_cast<std::underlying_type_t<Shutdown>>(shutdown));
      }))).map(std::function<Void(int)>([](auto n) { return Void(); }));
  ASYNC_TRY(res);
  INFO("shutdown {}\n", socket_);
  co_return Ok<Void, IoError>(Void());
}

auto net::TcpListener::bind(SocketAddr addr) -> Future<IoResult<TcpListener>> {
  using runtime::Handle;
  using runtime::Ready;

  const int max_pending_connection = 128;

  auto socket_result = TcpSocket::new_v4();
  if (socket_result.is_err()) {
    co_return Err<TcpListener, IoError>(socket_result.unwrap_err());
  }
  auto tcp_socket = socket_result.unwrap();
  auto bind_result = co_await tcp_socket.bind(addr);
  if (bind_result.is_err()) {
    co_return Err<TcpListener, IoError>(bind_result.unwrap_err());
  }
  co_return co_await tcp_socket.listen(max_pending_connection);
}

auto net::TcpListener::accept()
    -> Future<IoResult<std::pair<TcpStream, SocketAddr>>> {
  using CoOutput = IoResult<std::pair<TcpStream, SocketAddr>>;

  class Future : public runtime::Future<CoOutput> {
   public:
    explicit Future(TcpListener *self)
        : runtime::Future<CoOutput>(nullptr),
          self_(self),
          ready_(false),
          event_() {}

    [[nodiscard]] auto poll(runtime::Handle<void> self)
        -> runtime::Poll<CoOutput> override {
      if (!ready_) {
        event_ = reactor::Event{reactor::Interest::Read,
                                self_->socket_.into_c_fd(), this};
        auto res = self_->poll_->registry()->Register(&event_);
        if (res.is_err()) {
          if (res.unwrap_err().errno_ == EEXIST) {
            res = runtime::RuntimeCtx::get_ctx()
                      ->io_handle()
                      ->registry()
                      ->reregister(&event_);
          }
          if (res.is_err()) {
            return runtime::Ready<CoOutput>{
                Err<std::pair<TcpStream, SocketAddr>, IoError>(
                    res.unwrap_err())};
          }
        }
        ready_ = true;
        return runtime::Pending();
      }
      sockaddr_in addr_in{};
      socklen_t addrlen = sizeof(addr_in);
      auto res = into_sys_result(
          accept4(self_->socket_.into_c_fd(),
                  static_cast<sockaddr *>(static_cast<void *>(&addr_in)),
                  &addrlen, SOCK_NONBLOCK));
      if (res.is_err()) {
        return runtime::Ready<CoOutput>{
            Err<std::pair<TcpStream, SocketAddr>, IoError>(res.unwrap_err())};
      }
      auto fd = res.unwrap();
      std::string ip(INET_ADDRSTRLEN, 0);
      auto sock_addr = SocketAddr::new_v4(
          Ipv4Addr::New(inet_ntop(addr_in.sin_family, &addr_in.sin_addr,
                                  ip.data(), ip.size())),
          addr_in.sin_port);
      auto socket = Socket(fd);
      INFO("accept from {} new connect={{{}, addr:{}}}\n", self_->socket_,
           socket, sock_addr);
      return runtime::Ready<CoOutput>{
          Ok<std::pair<TcpStream, SocketAddr>, IoError>(
              {TcpStream(socket), sock_addr})};
    }

   private:
    TcpListener *self_;
    reactor::Event event_;
    bool ready_;
  };

  auto res = co_await Future(this);
  co_return res;
}

net::TcpListener::TcpListener(int fd)
    : socket_(fd), poll_(runtime::RuntimeCtx::get_ctx()->io_handle()) {}
