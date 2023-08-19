#include "xyco/io/read.h"
#include "xyco/io/registry.h"
#include "xyco/io/write.h"
#include "xyco/net/listener.h"
#include "xyco/runtime/runtime.h"
#include "xyco/task/registry.h"

class Server {
 public:
  Server(std::unique_ptr<xyco::runtime::Runtime> runtime, uint16_t port)
      : runtime_(std::move(runtime)), port_(port) {}

  auto run() -> void { runtime_->block_on(init_server()); }

 private:
  auto init_server() -> xyco::runtime::Future<void> {
    auto tcp_socket = *xyco::net::TcpSocket::new_v4();
    *tcp_socket.set_reuseaddr(true);
    *co_await tcp_socket.bind(xyco::net::SocketAddr::new_v4({}, port_));
    auto listener = *co_await tcp_socket.listen(LISTEN_BACKLOG);

    while (true) {
      auto server_stream = std::move((co_await listener.accept())->first);
      runtime_->spawn(echo(std::move(server_stream)));
    }
  }

  static auto echo(xyco::net::TcpStream server_stream)
      -> xyco::runtime::Future<void> {
    constexpr int buffer_size = 1024;

    std::string read_buf(buffer_size, 0);
    auto r_nbytes = *co_await xyco::io::ReadExt::read(server_stream, read_buf);
    if (r_nbytes == 0) {
      co_await server_stream.shutdown(xyco::io::Shutdown::All);
    } else {
      read_buf.resize(r_nbytes);
      *co_await xyco::io::WriteExt::write(server_stream, read_buf);
    }
  }

  std::unique_ptr<xyco::runtime::Runtime> runtime_;
  int port_;

  static constexpr int LISTEN_BACKLOG = 5000;
};

// NOLINTNEXTLINE(bugprone-exception-escape)
auto main() -> int {
  constexpr uint16_t port = 8080;

  auto server = Server(*xyco::runtime::Builder::new_multi_thread()
                            .worker_threads(2)
                            .registry<xyco::task::BlockingRegistry>(2)
                            .registry<xyco::io::IoRegistry>(4)
                            .build(),
                       port);
  server.run();
}
