#include <gtest/gtest.h>

#include "io/buffer_reader.h"
#include "io/write.h"
#include "utils.h"

class BufferTest : public ::testing::Test {
 protected:
  static void SetUpTestSuite() {
    TestRuntimeCtx::co_run([]() -> xyco::runtime::Future<void> {
      listener_ = std::make_unique<net::TcpListener>(
          net::TcpListener((co_await net::TcpListener::bind(
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
      client_ = std::make_unique<net::TcpStream>(
          net::TcpStream((co_await net::TcpStream::connect(
                              xyco::net::SocketAddr::new_v4(ip_, port_)))
                             .unwrap()));
      server_ = std::make_unique<net::TcpStream>(
          net::TcpStream((co_await listener_->accept()).unwrap().first));
    });
  }

  void TearDown() override {
    TestRuntimeCtx::co_run([&]() -> xyco::runtime::Future<void> {
      client_ = nullptr;
      server_ = nullptr;

      co_return;
    });
  }

  static std::unique_ptr<net::TcpListener> listener_;
  static const char *ip_;
  static const uint16_t port_;

  std::unique_ptr<net::TcpStream> client_;
  std::unique_ptr<net::TcpStream> server_;
};

std::unique_ptr<net::TcpListener> BufferTest::listener_;
const char *BufferTest::ip_ = "127.0.0.1";
const uint16_t BufferTest::port_ = 8088;

TEST_F(BufferTest, concept) {
  std::string_view string_view_buffer;
  xyco::io::WriteExt::write(*server_, string_view_buffer);
  std::array<char, 1> array_buffer{};
  xyco::io::ReadExt::read(*server_, array_buffer);
  std::span<char> span_buffer{};
  xyco::io::ReadExt::read(*server_, span_buffer);
  std::initializer_list<char> initializer_list_buffer;
  xyco::io::WriteExt::write(*server_, initializer_list_buffer);
  char c_array_buffer[1];
  xyco::io::ReadExt::read(*server_, c_array_buffer);

  xyco::io::BufferReader<net::TcpStream, std::string> reader(*server_);
  xyco::io::BufferReadExt::read_line<decltype(reader), std::string>(reader);
  xyco::io::BufferReadExt::read_line<decltype(reader), std::vector<char>>(
      reader);
}

TEST_F(BufferTest, read_to_end) {
  TestRuntimeCtx::co_run([&]() -> xyco::runtime::Future<void> {
    std::string_view write_bytes = "ab";
    auto write_nbytes =
        (co_await xyco::io::WriteExt::write(*client_, write_bytes)).unwrap();
    (co_await client_->shutdown(xyco::io::Shutdown::All)).unwrap();

    xyco::io::BufferReader<net::TcpStream, std::string> reader(*server_);
    auto readed =
        (co_await xyco::io::BufferReadExt::read_to_end<decltype(reader),
                                                       std::string>(reader))
            .unwrap();

    CO_ASSERT_EQ(write_nbytes, write_bytes.size());
    CO_ASSERT_EQ(readed, write_bytes);
  });
}

TEST_F(BufferTest, read_until) {
  TestRuntimeCtx::co_run([&]() -> xyco::runtime::Future<void> {
    std::string_view write_bytes = "abc";
    auto write_nbytes =
        (co_await xyco::io::WriteExt::write(*client_, write_bytes)).unwrap();
    (co_await client_->shutdown(xyco::io::Shutdown::All)).unwrap();

    xyco::io::BufferReader<net::TcpStream, std::string> reader(*server_);
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

    xyco::io::BufferReader<net::TcpStream, std::string> reader(*server_);
    auto line =
        (co_await xyco::io::BufferReadExt::read_until<decltype(reader),
                                                      std::string>(reader, 'c'))
            .unwrap();

    CO_ASSERT_EQ(write_nbytes, write_bytes.size());
    std::string expected_line(write_bytes);
    expected_line.push_back('\0');
    CO_ASSERT_EQ(line, expected_line);
  });
}

TEST_F(BufferTest, read_line) {
  TestRuntimeCtx::co_run([&]() -> xyco::runtime::Future<void> {
    std::string write_bytes = "ab\nc";
    auto write_nbytes =
        (co_await xyco::io::WriteExt::write(*client_, write_bytes)).unwrap();
    (co_await client_->shutdown(xyco::io::Shutdown::All)).unwrap();

    xyco::io::BufferReader<net::TcpStream, std::string> reader(*server_);
    auto line =
        (co_await xyco::io::BufferReadExt::read_line<decltype(reader),
                                                     std::string>(reader))
            .unwrap();

    CO_ASSERT_EQ(write_nbytes, write_bytes.size());
    CO_ASSERT_EQ(line, "ab\n");
  });
}

TEST_F(BufferTest, buffer_read) {
  TestRuntimeCtx::co_run([&]() -> xyco::runtime::Future<void> {
    std::string_view write_bytes = "abcde";
    auto _ =
        (co_await xyco::io::WriteExt::write(*client_, write_bytes)).unwrap();
    (co_await client_->shutdown(xyco::io::Shutdown::All)).unwrap();

    xyco::io::BufferReader<net::TcpStream, std::string> reader(*server_);
    auto readed = std::string(2, 0);
    auto read_bytes =
        (co_await reader.read(readed.begin(), readed.end())).unwrap();

    CO_ASSERT_EQ(read_bytes, readed.size());
    CO_ASSERT_EQ(readed, "ab");

    readed = std::string(3, 0);
    read_bytes = (co_await reader.read(readed.begin(), readed.end())).unwrap();

    CO_ASSERT_EQ(read_bytes, readed.size());
    CO_ASSERT_EQ(readed, "cde");
  });
}

TEST_F(BufferTest, c_array_buffer) {
  TestRuntimeCtx::co_run([&]() -> xyco::runtime::Future<void> {
    char write_bytes[] = "ab";
    (co_await xyco::io::WriteExt::write_all(*client_, write_bytes)).unwrap();
    (co_await client_->shutdown(xyco::io::Shutdown::All)).unwrap();

    char readed[4];
    auto readed_nbytes =
        (co_await xyco::io::ReadExt::read(*server_, readed)).unwrap();

    CO_ASSERT_EQ(std::string(readed), std::string(write_bytes));
  });
}