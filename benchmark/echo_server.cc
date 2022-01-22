#include "net/listener.h"
#include "runtime/runtime.h"

class Server {
 public:
  Server(std::unique_ptr<xyco::runtime::Runtime> runtime, const char *ip,
         uint16_t port)
      : runtime_(std::move(runtime)) {
    auto f = [=](const char *ip, uint16_t port) -> xyco::runtime::Future<void> {
      auto tcp_socket = xyco::net::TcpSocket::new_v4().unwrap();
      tcp_socket.set_reuseaddr(true).unwrap();
      (co_await tcp_socket.bind(xyco::net::SocketAddr::new_v4(ip, port)))
          .unwrap();
      auto listener = (co_await tcp_socket.listen(LISTEN_BACKLOG)).unwrap();

      while (true) {
        auto server_stream = (co_await listener.accept()).unwrap().first;
        runtime_->spawn(echo(std::move(server_stream)));
      }
    };
    runtime_->spawn(f(ip, port));
  }

 private:
  static auto echo(xyco::net::TcpStream server_stream)
      -> xyco::runtime::Future<void> {
    constexpr int buffer_size = 1024;

    std::vector<char> read_buf(buffer_size);
    auto r_nbytes = (co_await xyco::io::ReadExt<xyco::net::TcpStream>::read(
                         server_stream, read_buf))
                        .unwrap();
    if (r_nbytes == 0) {
      co_await server_stream.shutdown(xyco::io::Shutdown::All);
    } else {
      read_buf.resize(r_nbytes);
      (co_await xyco::io::WriteExt<xyco::net::TcpStream>::write(server_stream,
                                                                read_buf))
          .unwrap();
    }
  }

  std::unique_ptr<xyco::runtime::Runtime> runtime_;

  static constexpr int LISTEN_BACKLOG = 5000;
};

auto main() -> int {
  const char *ip = "127.0.0.1";
  constexpr uint16_t port = 8080;

  auto server = Server(xyco::runtime::Builder::new_multi_thread()
                           .worker_threads(1)
                           .max_blocking_threads(1)
                           .registry<xyco::io::IoRegistry>()
                           .build()
                           .unwrap(),
                       ip, port);

  while (true) {
    std::this_thread::sleep_for(std::chrono::seconds(3));
  }
}