#include "io/read.h"
#include "io/registry.h"
#include "io/write.h"
#include "net/listener.h"
#include "runtime/blocking.h"
#include "runtime/runtime.h"

class Server {
 public:
  Server(std::unique_ptr<xyco::runtime::Runtime> runtime, uint16_t port)
      : runtime_(std::move(runtime)) {
    runtime_->spawn(init_server(port));
  }

 private:
  auto init_server(uint16_t port) -> xyco::runtime::Future<void> {
    auto tcp_socket = xyco::net::TcpSocket::new_v4().unwrap();
    tcp_socket.set_reuseaddr(true).unwrap();
    (co_await tcp_socket.bind(xyco::net::SocketAddr::new_v4({}, port)))
        .unwrap();
    auto listener = (co_await tcp_socket.listen(LISTEN_BACKLOG)).unwrap();

    while (true) {
      auto server_stream = (co_await listener.accept()).unwrap().first;
      runtime_->spawn(echo(std::move(server_stream)));
    }
  }

  static auto echo(xyco::net::TcpStream server_stream)
      -> xyco::runtime::Future<void> {
    constexpr int buffer_size = 1024;

    std::string read_buf(buffer_size, 0);
    auto r_nbytes =
        (co_await xyco::io::ReadExt::read(server_stream, read_buf)).unwrap();
    if (r_nbytes == 0) {
      co_await server_stream.shutdown(xyco::io::Shutdown::All);
    } else {
      read_buf.resize(r_nbytes);
      (co_await xyco::io::WriteExt::write(server_stream, read_buf)).unwrap();
    }
  }

  std::unique_ptr<xyco::runtime::Runtime> runtime_;

  static constexpr int LISTEN_BACKLOG = 5000;
};

// NOLINTNEXTLINE(bugprone-exception-escape)
auto main() -> int {
  constexpr uint16_t port = 8080;
  // NOLINTNEXTLINE(clang-analyzer-deadcode.DeadStores)
  auto server = Server(xyco::runtime::Builder::new_multi_thread()
                           .worker_threads(2)
                           .registry<xyco::runtime::BlockingRegistry>(2)
                           .registry<xyco::io::IoRegistry>(4)
                           .build()
                           .unwrap(),
                       port);

  while (true) {
    std::this_thread::sleep_for(std::chrono::seconds(3));
  }
}