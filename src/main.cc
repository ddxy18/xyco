#include <chrono>
#include <vector>

#include "event/io/utils.h"
#include "event/net/listener.h"
#include "event/net/socket.h"
#include "event/runtime/future.h"
#include "event/runtime/runtime.h"
#include "fmt/core.h"
#include "fmt/format.h"
#include "utils/result.h"

using runtime::Future;

const int SERVER_PORT = 8080;

auto start_server() -> Future<void> {
  auto res = co_await net::TcpListener::bind(
      SocketAddr::new_v4(Ipv4Addr::New("127.0.0.1"), SERVER_PORT));
  if (res.is_err()) {
    auto err = res.unwrap_err();
    fmt::print("bind error:{}\n", err);
    co_return;
  }
  auto listener = res.unwrap();
  while (true) {
    auto [connection, addr] = (co_await listener.accept()).unwrap();
    auto buf = std::vector<char>({'a', 'b', 'c'});
    auto nbytes = (co_await connection.write(buf)).unwrap();
    fmt::print("success send \"{}\" to {}\n", fmt::join(buf, ""),
               connection.socket_.into_c_fd());
  }
  co_return;
}

auto start_client() -> Future<void> {
  auto connection = (co_await net::TcpStream::connect(
      SocketAddr::new_v4(Ipv4Addr::New("127.0.0.1"), SERVER_PORT)));
  while (connection.is_err()) {
    std::this_thread::sleep_for(std::chrono::seconds(1));
    connection = (co_await net::TcpStream::connect(
        SocketAddr::new_v4(Ipv4Addr::New("127.0.0.1"), SERVER_PORT)));
  }
  auto c = connection.unwrap();
  auto buf = std::vector<char>(10);
  auto nbytes = (co_await c.read(&buf)).unwrap();
  fmt::print("success read \"{}\"\n", fmt::join(buf, ""));
}

auto main(int /*unused*/, char** /*unused*/) -> int {
  auto rt = runtime::Builder::new_multi_thread()
                .worker_threads(2)
                .max_blocking_threads(2)
                .build()
                .unwrap();
  rt->run();
  rt->spawn(start_server());
  rt->spawn(start_client());
  rt->spawn(start_client());
  while (true) {
  }
  /* rt->spawn_blocking(std::function<int()>([]() {
    fmt::print("spawn blocing!");
    return 0;
  })); */
  while (true) {
  }
}
