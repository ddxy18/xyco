#include <gsl/pointers>

#include "asio/buffer.hpp"
#include "asio/co_spawn.hpp"
#include "asio/detached.hpp"
#include "asio/io_context.hpp"
#include "asio/ip/tcp.hpp"
#include "asio/read_until.hpp"
#include "asio/signal_set.hpp"
#include "asio/write.hpp"
class Server {
 public:
  Server(asio::io_context* context, uint16_t port) : context_(context) {
    auto init_server = [&](uint16_t port) -> asio::awaitable<void> {
      auto executor = co_await asio::this_coro::executor;
      asio::ip::tcp::acceptor acceptor(executor, {asio::ip::tcp::v4(), port});
      while (true) {
        auto socket = co_await acceptor.async_accept(asio::use_awaitable);
        asio::co_spawn(executor, echo(std::move(socket)), asio::detached);
      }
    };
    asio::co_spawn(*context_, init_server(port), asio::detached);
  }

 private:
  static auto echo(asio::ip::tcp::socket socket) -> asio::awaitable<void> {
    constexpr int buffer_size = 1024;

    std::string read_buf(buffer_size, 0);
    // NOLINTNEXTLINE(clang-analyzer-core.NullDereference)
    auto nbytes = co_await asio::async_read_until(
        socket, asio::dynamic_buffer(read_buf), '\n', asio::use_awaitable);
    co_await asio::async_write(socket, asio::buffer(read_buf, nbytes),
                               asio::use_awaitable);
  }

  gsl::not_null<asio::io_context*> context_;
};

// NOLINTNEXTLINE(bugprone-exception-escape)
auto main() -> int {
  constexpr uint16_t port = 8080;

  asio::io_context context(1);

  asio::signal_set signals(context, SIGINT, SIGTERM);
  signals.async_wait([&](auto, auto) { context.stop(); });

  auto server = Server(&context, port);

  context.run();

  return 0;
}
