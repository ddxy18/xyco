#include <gtest/gtest.h>

#include "io/buffer_reader.h"
#include "net/listener.h"
#include "utils.h"

TEST(BufferTest, buffer_read) {
  TestRuntimeCtx::co_run({[]() -> xyco::runtime::Future<void> {
    const char *ip = "127.0.0.1";
    const uint16_t port = 8087;

    auto server = (co_await xyco::net::TcpListener::bind(
                       xyco::net::SocketAddr::new_v4(ip, port)))
                      .unwrap();
    auto client = (co_await xyco::net::TcpStream::connect(
                       xyco::net::SocketAddr::new_v4(ip, port)))
                      .unwrap();
    auto [server_stream, addr] = (co_await server.accept()).unwrap();

    auto w_buf = {'a', 'b'};
    auto w_nbytes = (co_await xyco::io::WriteExt<xyco::net::TcpStream>::write(
                         client, w_buf))
                        .unwrap();
    (co_await client.shutdown(xyco::io::Shutdown::All)).unwrap();
    xyco::io::BufferReader<xyco::net::TcpStream> reader(
        std::move(server_stream));
    auto readed =
        (co_await xyco::io::ReadExt<decltype(reader)>::read_to_end(reader))
            .unwrap();

    CO_ASSERT_EQ(w_nbytes, w_buf.size());
    CO_ASSERT_EQ(std::string(readed.begin(), readed.end()),
                 std::string(w_buf.begin(), w_buf.end()));
  }});
}