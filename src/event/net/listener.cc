#include "listener.h"

#include <arpa/inet.h>
#include <asm-generic/errno-base.h>
#include <asm-generic/errno.h>
#include <asm-generic/socket.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cerrno>
#include <cstdint>
#include <cstring>
#include <functional>
#include <vector>

#include "event/io/utils.h"
#include "event/net/epoll.h"
#include "event/net/socket.h"
#include "event/runtime/async.h"
#include "event/runtime/future.h"
#include "event/runtime/poll.h"
#include "event/runtime/runtime.h"
#include "fmt/core.h"
#include "utils/result.h"

template <typename T>
using Future = runtime::Future<T>;

auto net::TcpStream::connect(SocketAddr addr) -> Future<IoResult<TcpStream>> {
  using CoOutput = IoResult<TcpStream>;

  class Future : public runtime::Future<CoOutput> {
   public:
    explicit Future(SocketAddr addr)
        : runtime::Future<CoOutput>(nullptr),
          ready_(false),
          sock_(Socket::new_nonblocking(addr, SOCK_STREAM)),
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
            auto res = runtime::runtime->io_handle()
                           ->registry()
                           ->Register(&event_)
                           .map(std::function<TcpStream(Void)>(
                               [](auto res) { return TcpStream(Socket()); }));
            if (res.is_err()) {
              return runtime::Ready<CoOutput>{res};
            }
            ready_ = true;
            return runtime::Pending();
          }
          fmt::print("connect fail fd:{}", sock_.into_c_fd());
          return runtime::Ready<CoOutput>{
              into_sys_result(c).map(std::function<TcpStream(int)>(
                  [](auto n) { return TcpStream(Socket()); }))};
        }
      }
      int ret = 0;
      socklen_t len = sizeof(decltype(ret));
      getsockopt(sock_.into_c_fd(), SOL_SOCKET, SO_ERROR, &ret, &len);
      if (ret != 0) {
        return runtime::Ready<CoOutput>{
            Err<TcpStream, IoError>(IoError{ret, strerror(ret)})};
      }
      fmt::print("{} connect to\n", sock_.into_c_fd());
      return runtime::Ready<CoOutput>{Ok<TcpStream, IoError>(TcpStream{sock_})};
    }

   private:
    bool ready_;
    Socket sock_;
    SocketAddr addr_;
    reactor::Event event_;
  };

  auto res = co_await Future(addr);
  co_return res;
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
        auto res =
            runtime::runtime->io_handle()->registry()->Register(&event_).map(
                std::function<uintptr_t(Void)>([](auto res) { return 0; }));
        if (res.is_err()) {
          if (res.unwrap_err().errno_ == EEXIST) {
            res = runtime::runtime->io_handle()
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
        auto res =
            runtime::runtime->io_handle()->registry()->reregister(&event_).map(
                std::function<uintptr_t(Void)>([](auto res) { return 0; }));
        return runtime::Pending();
      }
      auto nbytes = into_sys_result(n).map(std::function<uintptr_t(int)>(
          [](auto n) { return static_cast<uintptr_t>(n); }));
      fmt::print("read {} bytes\n", n);
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
        auto res =
            runtime::runtime->io_handle()->registry()->Register(&event_).map(
                std::function<uintptr_t(Void)>([](auto res) { return 0; }));
        if (res.is_err()) {
          if (res.unwrap_err().errno_ == EEXIST) {
            res = runtime::runtime->io_handle()
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
      fmt::print("write {} bytes\n", n);
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

auto net::TcpStream::shutdown(Shutdown shutdown) -> Future<IoResult<Void>> {
  auto res = into_sys_result(
      ::shutdown(socket_.into_c_fd(),
                 static_cast<std::underlying_type_t<Shutdown>>(shutdown)));
  co_return res.map(std::function<Void(int)>([](auto n) { return Void(); }));
}

auto net::TcpListener::bind(SocketAddr addr) -> Future<IoResult<TcpListener>> {
  using runtime::Handle;
  using runtime::Ready;

  const int max_pending_connection = 128;

  auto listener = TcpListener{};

  listener.socket_ = Socket::new_nonblocking(addr, SOCK_STREAM);
  listener.poll_ = runtime::runtime->io_handle();
  auto bind = co_await runtime::AsyncFuture(std::function<int()>([&]() {
    return ::bind(listener.socket_.into_c_fd(), addr.into_c_addr(),
                  sizeof(sockaddr));
  }));
  auto res = into_sys_result(bind).map(
      std::function<TcpListener(int)>([=](auto n) { return listener; }));
  ASYNC_TRY(res);
  fmt::print("bind fd:{}\n", listener.socket_.into_c_fd());
  auto listen = co_await runtime::AsyncFuture(std::function<int()>([&]() {
    return ::listen(listener.socket_.into_c_fd(), max_pending_connection);
  }));
  res = into_sys_result(listen).map(
      std::function<TcpListener(int)>([=](auto n) { return listener; }));
  ASYNC_TRY(res);
  fmt::print("listen fd:{}\n", listener.socket_.into_c_fd());
  co_return Ok<TcpListener, IoError>(listener);
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
        auto res = self_->poll_->registry()->Register(&event_).map(
            std::function<std::pair<TcpStream, SocketAddr>(Void)>([](auto res) {
              return std::pair{TcpStream(Socket()), SocketAddr()};
            }));
        if (res.is_err()) {
          if (res.unwrap_err().errno_ == EEXIST) {
            res =
                runtime::runtime->io_handle()
                    ->registry()
                    ->reregister(&event_)
                    .map(std::function<std::pair<TcpStream, SocketAddr>(Void)>(
                        [](auto res) {
                          return std::pair{TcpStream(Socket()), SocketAddr()};
                        }));
          }
          if (res.is_err()) {
            return runtime::Ready<CoOutput>{res};
          }
        }
        ready_ = true;
        return runtime::Pending();
      }
      sockaddr_in addr_in{};
      socklen_t addrlen = sizeof(addr_in);
      auto res = into_sys_result(accept4(self_->socket_.into_c_fd(),
                                         reinterpret_cast<sockaddr *>(&addr_in),
                                         &addrlen, SOCK_NONBLOCK));
      if (res.is_err()) {
        return runtime::Ready<CoOutput>{res.map(
            std::function<std::pair<TcpStream, SocketAddr>(int)>([](auto n) {
              return std::pair{TcpStream(Socket()), SocketAddr()};
            }))};
      }
      auto fd = res.unwrap();
      auto sock_addr = SocketAddr::new_v4(
          Ipv4Addr::New(inet_ntoa(addr_in.sin_addr)), addr_in.sin_port);
      auto socket = Socket::New(fd);
      fmt::print("accept {}\n", socket.into_c_fd());
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
