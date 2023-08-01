#include "io/read.h"
#include "io/registry.h"
#include "io/write.h"
#include "net/listener.h"
#include "runtime/runtime.h"
#include "task/registry.h"

using xyco::runtime::Future;

const std::string SERVER_IP = "127.0.0.1";
const int SERVER_PORT = 8080;

auto start_server() -> Future<void> {
  auto tcp_socket = xyco::net::TcpSocket::new_v4().unwrap();
  tcp_socket.set_reuseaddr(true).unwrap();

  auto bind_result = (co_await tcp_socket.bind(
      xyco::net::SocketAddr::new_v4({}, SERVER_PORT)));
  if (bind_result.is_err()) {
    auto err = bind_result.unwrap_err();
    ERROR("bind error:{}", err);
    co_return;
  }
  auto listener = (co_await tcp_socket.listen(3)).unwrap();

  while (true) {
    auto [connection, addr] = (co_await listener.accept()).unwrap();
    std::string_view buf = "abc";
    (co_await xyco::io::WriteExt::write(connection, buf)).unwrap();
    INFO("success send \"{}\" to {}\n", buf, connection);
  }
  co_return;
}

auto start_client() -> Future<void> {
  constexpr int max_buf_size = 10;

  auto start = std::chrono::system_clock::now();
  auto connect_result =
      (co_await xyco::net::TcpStream::connect(xyco::net::SocketAddr::new_v4(
          xyco::net::Ipv4Addr(SERVER_IP.c_str()), SERVER_PORT)));
  while (connect_result.is_err() &&
         std::chrono::system_clock::now() - start <= std::chrono::seconds(2)) {
    std::this_thread::sleep_for(std::chrono::seconds(1));
    connect_result =
        (co_await xyco::net::TcpStream::connect(xyco::net::SocketAddr::new_v4(
            xyco::net::Ipv4Addr(SERVER_IP.c_str()), SERVER_PORT)));
  }
  auto connection = connect_result.unwrap();
  auto buf = std::string(max_buf_size, 0);
  (co_await xyco::io::ReadExt::read(connection, buf)).unwrap();
  INFO("success read \"{}\" from {}\n", buf, connection);
}

// NOLINTNEXTLINE(bugprone-exception-escape)
auto main(int /*unused*/, char** /*unused*/) -> int {
  constexpr std::chrono::seconds wait_seconds(5);

  auto runtime = xyco::runtime::Builder::new_multi_thread()
                     .worker_threads(2)
                     .registry<xyco::task::BlockingRegistry>(2)
                     .registry<xyco::io::IoRegistry>(4)
                     .build()
                     .unwrap();
  runtime->spawn(start_server());
  // ensure server prepared to accept new connections
  std::this_thread::sleep_for(std::chrono::seconds(1));
  runtime->spawn(start_client());
  runtime->spawn(start_client());
  runtime->spawn(start_client());

  std::this_thread::sleep_for(wait_seconds);
}
