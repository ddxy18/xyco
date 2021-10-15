#include <chrono>
#include <thread>

#include "net/listener.h"
#include "runtime/runtime.h"

using xyco::runtime::Future;

const int SERVER_PORT = 8080;

auto start_server() -> Future<void> {
  auto res =
      co_await xyco::net::TcpListener::bind(xyco::net::SocketAddr::new_v4(
          xyco::net::Ipv4Addr("127.0.0.1"), SERVER_PORT));
  if (res.is_err()) {
    auto err = res.unwrap_err();
    ERROR("bind error:{}", err);
    co_return;
  }
  auto listener = res.unwrap();
  while (true) {
    auto [connection, addr] = (co_await listener.accept()).unwrap();
    auto buf = std::vector<char>({'a', 'b', 'c'});
    (co_await xyco::io::WriteExt<xyco::net::TcpStream>::write(connection, buf))
        .unwrap();
    INFO("success send \"{}\" to {}\n", fmt::join(buf, ""), connection);
  }
  co_return;
}

auto start_client() -> Future<void> {
  constexpr int max_buf_size = 10;

  auto start = std::chrono::system_clock::now();
  auto connection =
      (co_await xyco::net::TcpStream::connect(xyco::net::SocketAddr::new_v4(
          xyco::net::Ipv4Addr("127.0.0.1"), SERVER_PORT)));
  while (connection.is_err() &&
         std::chrono::system_clock::now() - start <= std::chrono::seconds(2)) {
    std::this_thread::sleep_for(std::chrono::seconds(1));
    connection =
        (co_await xyco::net::TcpStream::connect(xyco::net::SocketAddr::new_v4(
            xyco::net::Ipv4Addr("127.0.0.1"), SERVER_PORT)));
  }
  auto c = connection.unwrap();
  auto buf = std::vector<char>(max_buf_size);
  (co_await xyco::io::ReadExt<xyco::net::TcpStream>::read(c, buf)).unwrap();
  INFO("success read \"{}\" from {}\n", fmt::join(buf, ""), c);
}

auto main(int /*unused*/, char** /*unused*/) -> int {
  constexpr std::chrono::seconds wait_seconds(5);

  auto rt = xyco::runtime::Builder::new_multi_thread()
                .worker_threads(2)
                .max_blocking_threads(2)
                .build()
                .unwrap();
  rt->spawn(start_server());
  // ensure server prepared to accept new connections
  std::this_thread::sleep_for(std::chrono::seconds(1));
  rt->spawn(start_client());
  rt->spawn(start_client());
  rt->spawn(start_client());

  std::this_thread::sleep_for(wait_seconds);
}
