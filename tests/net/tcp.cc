#include <gtest/gtest.h>

#include "net/listener.h"
#include "utils.h"

// TODO(dongxiaoyu): add failure cases

TEST(TcpTest, reuseaddr) {
  TestRuntimeCtx::co_run([]() -> xyco::runtime::Future<void> {
    const char *ip = "127.0.0.1";
    const uint16_t port = 8081;

    {
      auto tcp_socket1 = xyco::net::TcpSocket::new_v4().unwrap();
      tcp_socket1.set_reuseaddr(true).unwrap();
      auto result1 =
          co_await tcp_socket1.bind(xyco::net::SocketAddr::new_v4(ip, port));
      CO_ASSERT_EQ(result1.is_ok(), true);
    }
    {
      auto tcp_socket2 = xyco::net::TcpSocket::new_v4().unwrap();
      tcp_socket2.set_reuseport(true).unwrap();
      auto result2 =
          co_await tcp_socket2.bind(xyco::net::SocketAddr::new_v4(ip, port));
      CO_ASSERT_EQ(result2.is_ok(), true);
    }
  });
}

TEST(TcpTest, reuseport) {
  TestRuntimeCtx::co_run([]() -> xyco::runtime::Future<void> {
    const char *ip = "127.0.0.1";
    const uint16_t port = 8082;

    auto tcp_socket1 = xyco::net::TcpSocket::new_v4().unwrap();
    tcp_socket1.set_reuseport(true).unwrap();
    auto result1 =
        co_await tcp_socket1.bind(xyco::net::SocketAddr::new_v4(ip, port));

    auto tcp_socket2 = xyco::net::TcpSocket::new_v4().unwrap();
    tcp_socket2.set_reuseport(true).unwrap();
    auto result2 =
        co_await tcp_socket2.bind(xyco::net::SocketAddr::new_v4(ip, port));

    CO_ASSERT_EQ(result1.is_ok(), true);
    CO_ASSERT_EQ(result2.is_ok(), true);
  });
}

TEST(TcpTest, bind_same_addr) {
  TestRuntimeCtx::co_run([]() -> xyco::runtime::Future<void> {
    const char *ip = "127.0.0.1";
    const uint16_t port = 8083;

    auto result1 = co_await xyco::net::TcpListener::bind(
        xyco::net::SocketAddr::new_v4(ip, port));

    auto result2 = co_await xyco::net::TcpListener::bind(
        xyco::net::SocketAddr::new_v4(ip, port));

    CO_ASSERT_EQ(result1.is_ok(), true);
    CO_ASSERT_EQ(result2.is_err(), true);
  });
}

TEST(TcpTest, TcpSocket_listen) {
  TestRuntimeCtx::co_run([]() -> xyco::runtime::Future<void> {
    const char *ip = "127.0.0.1";
    const uint16_t port = 8084;

    auto tcp_socket = xyco::net::TcpSocket::new_v4().unwrap();
    (co_await tcp_socket.bind(xyco::net::SocketAddr::new_v4(ip, port)))
        .unwrap();
    auto listener = co_await tcp_socket.listen(1);

    CO_ASSERT_EQ(listener.is_ok(), true);
  });
}

TEST(TcpTest, TcpListener_bind) {
  TestRuntimeCtx::co_run([]() -> xyco::runtime::Future<void> {
    const char *ip = "127.0.0.1";
    const uint16_t port = 8085;

    auto result = co_await xyco::net::TcpListener::bind(
        xyco::net::SocketAddr::new_v4(ip, port));

    CO_ASSERT_EQ(result.is_ok(), true);
  });
}

TEST(TcpTest, connect_to_closed_server) {
  TestRuntimeCtx::co_run([]() -> xyco::runtime::Future<void> {
    const char *ip = "127.0.0.1";
    const uint16_t port = 9000;

    auto client = co_await xyco::net::TcpStream::connect(
        xyco::net::SocketAddr::new_v4(ip, port));

    CO_ASSERT_EQ(client.unwrap_err().errno_, ECONNREFUSED);
  });
}

class WithServerTest : public ::testing::Test {
 protected:
  static void SetUpTestSuite() {
    TestRuntimeCtx::co_run([]() -> xyco::runtime::Future<void> {
      auto tcp_socket = xyco::net::TcpSocket::new_v4().unwrap();
      tcp_socket.set_reuseaddr(true).unwrap();
      (co_await tcp_socket.bind(xyco::net::SocketAddr::new_v4(ip_, port_)))
          .unwrap();
      listener_ =
          new xyco::net::TcpListener((co_await tcp_socket.listen(1)).unwrap());
    });
  }

  static void TearDownTestSuite() {
    TestRuntimeCtx::co_run([]() -> xyco::runtime::Future<void> {
      delete listener_;
      co_return;
    });
  }

  static gsl::owner<xyco::net::TcpListener *> listener_;
  static const char *ip_;
  static const uint16_t port_;
};

gsl::owner<xyco::net::TcpListener *> WithServerTest::listener_;
const char *WithServerTest::ip_ = "127.0.0.1";
const uint16_t WithServerTest::port_ = 8080;

TEST_F(WithServerTest, TcpSocket_connect) {
  TestRuntimeCtx::co_run([]() -> xyco::runtime::Future<void> {
    auto client = co_await xyco::net::TcpSocket::new_v4().unwrap().connect(
        xyco::net::SocketAddr::new_v4(ip_, port_));

    (co_await listener_->accept()).unwrap();

    CO_ASSERT_EQ(client.is_ok(), true);
  });
}

TEST_F(WithServerTest, TcpSocket_new_v6) {
  ASSERT_EQ(xyco::net::TcpSocket::new_v6().is_ok(), true);
}

TEST_F(WithServerTest, TcpStream_connect) {
  TestRuntimeCtx::co_run([]() -> xyco::runtime::Future<void> {
    auto client = co_await xyco::net::TcpStream::connect(
        xyco::net::SocketAddr::new_v4(ip_, port_));

    (co_await listener_->accept()).unwrap();

    CO_ASSERT_EQ(client.is_ok(), true);
  });
}

TEST_F(WithServerTest, TcpStream_rw) {
  TestRuntimeCtx::co_run([]() -> xyco::runtime::Future<void> {
    auto client = (co_await xyco::net::TcpStream::connect(
                       xyco::net::SocketAddr::new_v4(ip_, port_)))
                      .unwrap();
    auto [server_stream, addr] = (co_await listener_->accept()).unwrap();

    auto w_buf = {'a'};
    auto w_nbytes = (co_await xyco::io::WriteExt<xyco::net::TcpStream>::write(
                         client, w_buf))
                        .unwrap();
    auto r_buf = std::vector<char>(w_buf.size());
    auto r_nbytes = (co_await xyco::io::ReadExt<xyco::net::TcpStream>::read(
                         server_stream, r_buf))
                        .unwrap();

    CO_ASSERT_EQ(w_nbytes, w_buf.size());
    CO_ASSERT_EQ(r_nbytes, r_buf.size());
  });
}

TEST_F(WithServerTest, TcpListener_accept) {
  TestRuntimeCtx::co_run([]() -> xyco::runtime::Future<void> {
    auto client = (co_await xyco::net::TcpSocket::new_v4().unwrap().connect(
                       xyco::net::SocketAddr::new_v4(ip_, port_)))
                      .unwrap();
    auto [stream, addr] = (co_await listener_->accept()).unwrap();
    const auto *raw_addr = static_cast<const sockaddr_in *>(
        static_cast<const void *>(addr.into_c_addr()));

    CO_ASSERT_EQ(raw_addr->sin_addr.s_addr, inet_addr(ip_));
  });
}