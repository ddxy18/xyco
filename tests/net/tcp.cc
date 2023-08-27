#include <gtest/gtest.h>

#include "utils.h"
#include "xyco/io/read.h"
#include "xyco/io/write.h"

// TODO(dongxiaoyu): add failure cases

TEST(TcpTest, reuseaddr) {
  TestRuntimeCtx::runtime()->block_on([]() -> xyco::runtime::Future<void> {
    const uint16_t port = 8081;

    {
      auto tcp_socket1 = *xyco::net::TcpSocket::new_v4();
      *tcp_socket1.set_reuseaddr(true);
      auto result1 =
          co_await tcp_socket1.bind(xyco::net::SocketAddr::new_v4({}, port));
      CO_ASSERT_EQ(result1.has_value(), true);
    }
    {
      auto tcp_socket2 = *xyco::net::TcpSocket::new_v4();
      *tcp_socket2.set_reuseport(true);
      auto result2 =
          co_await tcp_socket2.bind(xyco::net::SocketAddr::new_v4({}, port));
      CO_ASSERT_EQ(result2.has_value(), true);
    }
  }());
}

TEST(TcpTest, reuseport) {
  TestRuntimeCtx::runtime()->block_on([]() -> xyco::runtime::Future<void> {
    const uint16_t port = 8082;

    auto tcp_socket1 = *xyco::net::TcpSocket::new_v4();
    *tcp_socket1.set_reuseport(true);
    auto result1 =
        co_await tcp_socket1.bind(xyco::net::SocketAddr::new_v4({}, port));

    auto tcp_socket2 = *xyco::net::TcpSocket::new_v4();
    *tcp_socket2.set_reuseport(true);
    auto result2 =
        co_await tcp_socket2.bind(xyco::net::SocketAddr::new_v4({}, port));

    CO_ASSERT_EQ(result1.has_value(), true);
    CO_ASSERT_EQ(result2.has_value(), true);
  }());
}

TEST(TcpTest, bind_same_addr) {
  TestRuntimeCtx::runtime()->block_on([]() -> xyco::runtime::Future<void> {
    const uint16_t port = 8083;

    auto result1 = co_await xyco::net::TcpListener::bind(
        xyco::net::SocketAddr::new_v4({}, port));

    auto result2 = co_await xyco::net::TcpListener::bind(
        xyco::net::SocketAddr::new_v4({}, port));

    CO_ASSERT_EQ(result1.has_value(), true);
    CO_ASSERT_EQ(result2.has_value(), false);
  }());
}

TEST(TcpTest, TcpSocket_listen) {
  TestRuntimeCtx::runtime()->block_on([]() -> xyco::runtime::Future<void> {
    // Skip 8084 since the port is occupied by github runner.
    const uint16_t port = 8085;

    auto tcp_socket = *xyco::net::TcpSocket::new_v4();
    *co_await tcp_socket.bind(xyco::net::SocketAddr::new_v4({}, port));
    auto listener = co_await tcp_socket.listen(1);

    CO_ASSERT_EQ(listener.has_value(), true);
  }());
}

TEST(TcpTest, TcpListener_bind) {
  TestRuntimeCtx::runtime()->block_on([]() -> xyco::runtime::Future<void> {
    const uint16_t port = 8086;

    auto result = co_await xyco::net::TcpListener::bind(
        xyco::net::SocketAddr::new_v4({}, port));

    CO_ASSERT_EQ(result.has_value(), true);
  }());
}

TEST(TcpTest, connect_to_closed_server) {
  TestRuntimeCtx::runtime()->block_on([]() -> xyco::runtime::Future<void> {
    const char *ip = "127.0.0.1";
    const uint16_t port = 9000;

    auto client = co_await xyco::net::TcpStream::connect(
        xyco::net::SocketAddr::new_v4(ip, port));

    CO_ASSERT_EQ(client.error().errno_, ECONNREFUSED);
  }());
}

class WithServerTest : public ::testing::Test {
 protected:
  static void SetUpTestSuite() {
    TestRuntimeCtx::runtime()->block_on([]() -> xyco::runtime::Future<void> {
      auto tcp_socket = *xyco::net::TcpSocket::new_v4();
      *tcp_socket.set_reuseaddr(true);
      *co_await tcp_socket.bind(xyco::net::SocketAddr::new_v4({}, port_));
      listener_ = std::make_unique<xyco::net::TcpListener>(
          xyco::net::TcpListener(*co_await tcp_socket.listen(1)));
    }());
  }

  static void TearDownTestSuite() {
    TestRuntimeCtx::runtime()->block_on([]() -> xyco::runtime::Future<void> {
      listener_ = nullptr;

      co_return;
    }());
  }

  static std::unique_ptr<xyco::net::TcpListener> listener_;
  static const char *ip_;
  static const uint16_t port_;
};

std::unique_ptr<xyco::net::TcpListener> WithServerTest::listener_;
const char *WithServerTest::ip_ = "127.0.0.1";
const uint16_t WithServerTest::port_ = 8080;

TEST_F(WithServerTest, TcpSocket_connect) {
  TestRuntimeCtx::runtime()->block_on([]() -> xyco::runtime::Future<void> {
    auto client = co_await xyco::net::TcpSocket::new_v4()->connect(
        xyco::net::SocketAddr::new_v4(ip_, port_));

    *co_await listener_->accept();

    CO_ASSERT_EQ(client.has_value(), true);
  }());
}

TEST_F(WithServerTest, TcpStream_flush) {
  TestRuntimeCtx::runtime()->block_on([]() -> xyco::runtime::Future<void> {
    auto client = *co_await xyco::net::TcpSocket::new_v4()->connect(
        xyco::net::SocketAddr::new_v4(ip_, port_));
    *co_await listener_->accept();
    auto flush_result = co_await client.flush();

    CO_ASSERT_EQ(flush_result.has_value(), true);
  }());
}

TEST_F(WithServerTest, TcpSocket_new_v6) {
  ASSERT_EQ(xyco::net::TcpSocket::new_v6().has_value(), true);
}

TEST_F(WithServerTest, TcpStream_connect) {
  TestRuntimeCtx::runtime()->block_on([]() -> xyco::runtime::Future<void> {
    auto client = co_await xyco::net::TcpStream::connect(
        xyco::net::SocketAddr::new_v4(ip_, port_));

    *co_await listener_->accept();

    CO_ASSERT_EQ(client.has_value(), true);
  }());
}

TEST_F(WithServerTest, TcpStream_rw) {
  TestRuntimeCtx::runtime()->block_on([]() -> xyco::runtime::Future<void> {
    auto client = *co_await xyco::net::TcpStream::connect(
        xyco::net::SocketAddr::new_v4(ip_, port_));
    auto [server_stream, addr] = *co_await listener_->accept();

    std::string_view w_buf = "a";
    auto w_nbytes = *co_await xyco::io::WriteExt::write(client, w_buf);
    auto r_buf = std::string(w_buf.size(), 0);
    auto r_nbytes = *co_await xyco::io::ReadExt::read(server_stream, r_buf);

    CO_ASSERT_EQ(w_nbytes, w_buf.size());
    CO_ASSERT_EQ(r_nbytes, r_buf.size());
  }());
}

TEST_F(WithServerTest, TcpListener_accept) {
  TestRuntimeCtx::runtime()->block_on([]() -> xyco::runtime::Future<void> {
    auto client = *co_await xyco::net::TcpSocket::new_v4()->connect(
        xyco::net::SocketAddr::new_v4(ip_, port_));
    auto [stream, addr] = *co_await listener_->accept();
    const auto *raw_addr = static_cast<const sockaddr_in *>(
        static_cast<const void *>(addr.into_c_addr()));

    CO_ASSERT_EQ(raw_addr->sin_addr.s_addr, inet_addr(ip_));
  }());
}

TEST_F(WithServerTest, TcpStream_rw_loop) {
  constexpr int ITERATION_TIMES = 100000;

#ifdef XYCO_ENABLE_LOG
  auto original_level = LoggerCtx::get_logger()->level();
  LoggerCtx::get_logger()->set_level(spdlog::level::off);
#endif
  TestRuntimeCtx::runtime()->block_on([]() -> xyco::runtime::Future<void> {
    auto client = *co_await xyco::net::TcpStream::connect(
        xyco::net::SocketAddr::new_v4(ip_, port_));
    auto [server_stream, addr] = *co_await listener_->accept();

    for (int i = 0; i < ITERATION_TIMES; i++) {
      std::string_view w_buf = "a";
      auto w_nbytes = *co_await xyco::io::WriteExt::write(client, w_buf);
      auto r_buf = std::string(w_buf.size(), 0);
      auto r_nbytes = *co_await xyco::io::ReadExt::read(server_stream, r_buf);

      CO_ASSERT_EQ(w_nbytes, w_buf.size());
      CO_ASSERT_EQ(r_nbytes, r_buf.size());
    }
  }());
#ifdef XYCO_ENABLE_LOG
  LoggerCtx::get_logger()->set_level(original_level);
#endif
}

TEST_F(WithServerTest, TcpStream_rw_twice) {
  TestRuntimeCtx::runtime()->block_on([]() -> xyco::runtime::Future<void> {
    auto client = *co_await xyco::net::TcpStream::connect(
        xyco::net::SocketAddr::new_v4(ip_, port_));
    auto [server_stream, addr] = *co_await listener_->accept();

    std::string_view w_buf = "a";
    auto w_nbytes = *co_await xyco::io::WriteExt::write(client, w_buf);
    auto r_buf = std::string(w_buf.size(), 0);
    auto r_nbytes = *co_await xyco::io::ReadExt::read(server_stream, r_buf);

    CO_ASSERT_EQ(w_nbytes, w_buf.size());
    CO_ASSERT_EQ(r_nbytes, r_buf.size());

    w_nbytes = *co_await xyco::io::WriteExt::write(client, w_buf);
    r_nbytes = *co_await xyco::io::ReadExt::read(server_stream, r_buf);

    CO_ASSERT_EQ(w_nbytes, w_buf.size());
    CO_ASSERT_EQ(r_nbytes, r_buf.size());
  }());
}
