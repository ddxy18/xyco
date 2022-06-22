#include <gtest/gtest.h>

#include "io/buffer_reader.h"
#include "net/listener.h"
#include "utils.h"

class BufferTest : public ::testing::Test {
 protected:
  static void SetUpTestSuite() {
    TestRuntimeCtx::co_run([]() -> xyco::runtime::Future<void> {
      listener_ =
          std::make_unique<xyco::net::TcpListener>(xyco::net::TcpListener(
              (co_await xyco::net::TcpListener::bind(
                   xyco::net::SocketAddr::new_v4(ip_, port_)))
                  .unwrap()));
    });
  }

  static void TearDownTestSuite() {
    TestRuntimeCtx::co_run([]() -> xyco::runtime::Future<void> {
      listener_ = nullptr;

      co_return;
    });
  }

  void SetUp() override {
    TestRuntimeCtx::co_run([&]() -> xyco::runtime::Future<void> {
      client_ = std::make_unique<xyco::net::TcpStream>(
          xyco::net::TcpStream((co_await xyco::net::TcpStream::connect(
                                    xyco::net::SocketAddr::new_v4(ip_, port_)))
                                   .unwrap()));
      server_ = std::make_unique<xyco::net::TcpStream>(
          xyco::net::TcpStream((co_await listener_->accept()).unwrap().first));
    });
  }

  void TearDown() override {
    TestRuntimeCtx::co_run([&]() -> xyco::runtime::Future<void> {
      client_ = nullptr;
      server_ = nullptr;

      co_return;
    });
  }

  static std::unique_ptr<xyco::net::TcpListener> listener_;
  static const char *ip_;
  static const uint16_t port_;

  std::unique_ptr<xyco::net::TcpStream> client_;
  std::unique_ptr<xyco::net::TcpStream> server_;
};

std::unique_ptr<xyco::net::TcpListener> BufferTest::listener_;
const char *BufferTest::ip_ = "127.0.0.1";
const uint16_t BufferTest::port_ = 8088;

TEST_F(BufferTest, buffer_read) {
  TestRuntimeCtx::co_run([&]() -> xyco::runtime::Future<void> {
    std::string_view write_bytes = "ab";
    auto write_nbytes =
        (co_await xyco::io::WriteExt::write(*client_, write_bytes)).unwrap();
    (co_await client_->shutdown(xyco::io::Shutdown::All)).unwrap();

    xyco::io::BufferReader<xyco::net::TcpStream, std::string> reader(*server_);
    auto readed =
        (co_await xyco::io::ReadExt::read_to_end<decltype(reader), std::string>(
             reader))
            .unwrap();

    CO_ASSERT_EQ(write_nbytes, write_bytes.size());
    CO_ASSERT_EQ(readed, std::string(write_bytes.begin(), write_bytes.end()));
  });
}

TEST_F(BufferTest, read_until) {
  TestRuntimeCtx::co_run([&]() -> xyco::runtime::Future<void> {
    std::string_view write_bytes = "abc";
    auto write_nbytes =
        (co_await xyco::io::WriteExt::write(*client_, write_bytes)).unwrap();
    (co_await client_->shutdown(xyco::io::Shutdown::All)).unwrap();

    xyco::io::BufferReader<xyco::net::TcpStream, std::string> reader(*server_);
    auto line =
        (co_await xyco::io::BufferReadExt::read_until<decltype(reader),
                                                      std::string>(reader, 'c'))
            .unwrap();

    CO_ASSERT_EQ(write_nbytes, write_bytes.size());
    CO_ASSERT_EQ(line, write_bytes);
  });
}

TEST_F(BufferTest, read_until_eof) {
  TestRuntimeCtx::co_run([&]() -> xyco::runtime::Future<void> {
    std::string_view write_bytes = "ab";
    auto write_nbytes =
        (co_await xyco::io::WriteExt::write(*client_, write_bytes)).unwrap();
    (co_await client_->shutdown(xyco::io::Shutdown::All)).unwrap();

    xyco::io::BufferReader<xyco::net::TcpStream, std::string> reader(*server_);
    auto line =
        (co_await xyco::io::BufferReadExt::read_until<decltype(reader),
                                                      std::string>(reader, 'c'))
            .unwrap();

    CO_ASSERT_EQ(write_nbytes, write_bytes.size());
    CO_ASSERT_EQ(line, write_bytes);
  });
}

TEST_F(BufferTest, read_line) {
  TestRuntimeCtx::co_run([&]() -> xyco::runtime::Future<void> {
    std::string write_bytes = "ab\nc";
    auto stream = std::istringstream(write_bytes);
    std::string w_line;
    stream >> w_line;
    auto write_nbytes =
        (co_await xyco::io::WriteExt::write(*client_, write_bytes)).unwrap();
    (co_await client_->shutdown(xyco::io::Shutdown::All)).unwrap();

    xyco::io::BufferReader<xyco::net::TcpStream, std::string> reader(*server_);
    auto line =
        (co_await xyco::io::BufferReadExt::read_line<decltype(reader),
                                                     std::string>(reader))
            .unwrap();

    CO_ASSERT_EQ(write_nbytes, write_bytes.size());
    CO_ASSERT_EQ(line, w_line);
  });
}