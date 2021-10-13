#include "net/listener.h"

#include <gtest/gtest.h>

#include "utils.h"

class ListenerTest : public ::testing::Test {
 protected:
  void SetUp() override {}
};

// TODO(dongxiaoyu): add failure cases

TEST_F(ListenerTest, bind_same_addr) {
  TestRuntimeCtx::co_run({[]() -> runtime::Future<void> {
    const char *ip = "127.0.0.1";
    const uint16_t port = 8080;

    auto tcp_socket1 = net::TcpSocket::new_v4().unwrap();
    auto result1 = co_await tcp_socket1.bind(net::SocketAddr::new_v4(ip, port));

    auto tcp_socket2 = net::TcpSocket::new_v4().unwrap();
    auto result2 = co_await tcp_socket2.bind(net::SocketAddr::new_v4(ip, port));

    CO_ASSERT_EQ(result1.is_ok(), true);
    CO_ASSERT_EQ(result2.is_err(), true);
  }});
}

TEST_F(ListenerTest, TcpSocket_connect) {
  TestRuntimeCtx::co_run({[]() -> runtime::Future<void> {
    const char *ip = "127.0.0.1";
    const uint16_t port = 8081;

    auto server =
        (co_await net::TcpListener::bind(net::SocketAddr::new_v4(ip, port)))
            .unwrap();
    auto client = co_await net::TcpSocket::new_v4().unwrap().connect(
        net::SocketAddr::new_v4(ip, port));

    CO_ASSERT_EQ(client.is_ok(), true);
  }});
}

TEST_F(ListenerTest, TcpSocket_listen) {
  TestRuntimeCtx::co_run({[]() -> runtime::Future<void> {
    const char *ip = "127.0.0.1";
    const uint16_t port = 8082;

    auto tcp_socket = net::TcpSocket::new_v4().unwrap();
    (co_await tcp_socket.bind(net::SocketAddr::new_v4(ip, port))).unwrap();
    auto listener = co_await tcp_socket.listen(10);

    CO_ASSERT_EQ(listener.is_ok(), true);
  }});
}

TEST_F(ListenerTest, TcpSocket_new_v6) {
  ASSERT_EQ(net::TcpSocket::new_v6().is_ok(), true);
}

TEST_F(ListenerTest, TcpStream_connect) {
  TestRuntimeCtx::co_run({[]() -> runtime::Future<void> {
    const char *ip = "127.0.0.1";
    const uint16_t port = 8083;

    auto server =
        (co_await net::TcpListener::bind(net::SocketAddr::new_v4(ip, port)))
            .unwrap();
    auto client =
        co_await net::TcpStream::connect(net::SocketAddr::new_v4(ip, port));

    CO_ASSERT_EQ(client.is_ok(), true);
  }});
}

TEST_F(ListenerTest, TcpStream_rw) {
  TestRuntimeCtx::co_run({[]() -> runtime::Future<void> {
    const char *ip = "127.0.0.1";
    const uint16_t port = 8084;

    auto server =
        (co_await net::TcpListener::bind(net::SocketAddr::new_v4(ip, port)))
            .unwrap();
    auto client =
        (co_await net::TcpStream::connect(net::SocketAddr::new_v4(ip, port)))
            .unwrap();
    auto [server_stream, addr] = (co_await server.accept()).unwrap();

    auto w_buf = {'a'};
    auto w_nbytes =
        (co_await io::WriteExt<net::TcpStream>::write(client, w_buf)).unwrap();
    auto r_buf = std::vector<char>(w_buf.size());
    auto r_nbytes =
        (co_await io::ReadExt<net::TcpStream>::read(server_stream, r_buf))
            .unwrap();

    CO_ASSERT_EQ(w_nbytes, w_buf.size());
    CO_ASSERT_EQ(r_nbytes, r_buf.size());
  }});
}

TEST_F(ListenerTest, TcpListener_bind) {
  TestRuntimeCtx::co_run({[]() -> runtime::Future<void> {
    const char *ip = "127.0.0.1";
    const uint16_t port = 8085;

    auto result =
        co_await net::TcpListener::bind(net::SocketAddr::new_v4(ip, port));

    CO_ASSERT_EQ(result.is_ok(), true);
  }});
}

TEST_F(ListenerTest, TcpListener_accept) {
  TestRuntimeCtx::co_run({[]() -> runtime::Future<void> {
    const char *ip = "127.0.0.1";
    const uint16_t port = 8086;

    auto server =
        (co_await net::TcpListener::bind(net::SocketAddr::new_v4(ip, port)))
            .unwrap();
    auto client = (co_await net::TcpSocket::new_v4().unwrap().connect(
                       net::SocketAddr::new_v4(ip, port)))
                      .unwrap();
    auto [stream, addr] = (co_await server.accept()).unwrap();
    const auto *raw_addr = static_cast<const sockaddr_in *>(
        static_cast<const void *>(addr.into_c_addr()));

    CO_ASSERT_EQ(raw_addr->sin_addr.s_addr, inet_addr(ip));
  }});
}