#include <chrono>
#include <thread>

#include "net/listener.h"
#include "runtime/runtime.h"

using runtime::Future;

const int SERVER_PORT = 8080;

auto start_server() -> Future<void> {
  auto res = co_await net::TcpListener::bind(
      net::SocketAddr::new_v4(net::Ipv4Addr("127.0.0.1"), SERVER_PORT));
  if (res.is_err()) {
    auto err = res.unwrap_err();
    ERROR("bind error:{}", err);
    co_return;
  }
  auto listener = res.unwrap();
  while (true) {
    auto [connection, addr] = (co_await listener.accept()).unwrap();
    auto buf = std::vector<char>({'a', 'b', 'c'});
    (co_await io::WriteExt<net::TcpStream>::write(connection, buf)).unwrap();
    INFO("success send \"{}\" to {}", fmt::join(buf, ""), connection);
  }
  co_return;
}

auto start_client() -> Future<void> {
  constexpr int max_buf_size = 10;

  auto start = std::chrono::system_clock::now();
  auto connection = (co_await net::TcpStream::connect(
      net::SocketAddr::new_v4(net::Ipv4Addr("127.0.0.1"), SERVER_PORT)));
  while (connection.is_err() &&
         std::chrono::system_clock::now() - start <= std::chrono::seconds(2)) {
    std::this_thread::sleep_for(std::chrono::seconds(1));
    connection = (co_await net::TcpStream::connect(
        net::SocketAddr::new_v4(net::Ipv4Addr("127.0.0.1"), SERVER_PORT)));
  }
  auto c = connection.unwrap();
  auto buf = std::vector<char>(max_buf_size);
  (co_await io::ReadExt<net::TcpStream>::read(c, buf)).unwrap();
  INFO("success read \"{}\" from {}", fmt::join(buf, ""), c);
}

auto main(int /*unused*/, char** /*unused*/) -> int {
  constexpr std::chrono::seconds wait_seconds(5);

  auto rt = runtime::Builder::new_multi_thread()
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
