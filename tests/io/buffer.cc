#include <gtest/gtest.h>

#include <coroutine>

import xyco.test.utils;
import xyco.net;
import xyco.io;

class BufferTest : public ::testing::Test {
 protected:
  static void SetUpTestSuite() {
    TestRuntimeCtx::runtime()->block_on([]() -> xyco::runtime::Future<void> {
      listener_ = std::make_unique<xyco::net::TcpListener>(xyco::net::TcpListener(
          *co_await xyco::net::TcpListener::bind(xyco::net::SocketAddr::new_v4({}, port_))));
    }());
  }

  static void TearDownTestSuite() {
    TestRuntimeCtx::runtime()->block_on([]() -> xyco::runtime::Future<void> {
      listener_ = nullptr;

      co_return;
    }());
  }

  void SetUp() override {
    // NOLINTNEXTLINE(cppcoreguidelines-avoid-capturing-lambda-coroutines)
    TestRuntimeCtx::runtime()->block_on([&]() -> xyco::runtime::Future<void> {
      client_ = std::make_unique<xyco::net::TcpStream>(xyco::net::TcpStream(
          *co_await xyco::net::TcpStream::connect(xyco::net::SocketAddr::new_v4(ip_, port_))));
      server_ = std::make_unique<xyco::net::TcpStream>(
          xyco::net::TcpStream(std::move((co_await listener_->accept())->first)));
    }());
  }

  void TearDown() override {
    // NOLINTNEXTLINE(cppcoreguidelines-avoid-capturing-lambda-coroutines)
    TestRuntimeCtx::runtime()->block_on([&]() -> xyco::runtime::Future<void> {
      client_ = nullptr;
      server_ = nullptr;

      co_return;
    }());
  }

  static std::unique_ptr<xyco::net::TcpListener> listener_;
  static const char *ip_;
  static const uint16_t port_;

 public:
  std::unique_ptr<xyco::net::TcpStream> client_;
  std::unique_ptr<xyco::net::TcpStream> server_;
};

std::unique_ptr<xyco::net::TcpListener> BufferTest::listener_;
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
  // NOLINTNEXTLINE(modernize-avoid-c-arrays)
  char c_array_buffer[1];
  xyco::io::ReadExt::read(*server_, c_array_buffer);

  xyco::io::BufferReader<xyco::net::TcpStream, std::string> reader(server_.get());
  xyco::io::BufferReadExt::read_line<decltype(reader), std::string>(reader);
  xyco::io::BufferReadExt::read_line<decltype(reader), std::vector<char>>(reader);
}

TEST_F(BufferTest, read_to_end) {
  TestRuntimeCtx::runtime()->block_on(
      [](xyco::net::TcpStream *client,
         xyco::net::TcpStream *server) -> xyco::runtime::Future<void> {
        std::string_view write_bytes = "ab";
        auto write_nbytes = *co_await xyco::io::WriteExt::write(*client, write_bytes);
        *co_await client->shutdown(xyco::io::Shutdown::All);

        xyco::io::BufferReader<xyco::net::TcpStream, std::string> reader(server);
        auto readed =
            *co_await xyco::io::BufferReadExt::read_to_end<decltype(reader), std::string>(reader);

        CO_ASSERT_EQ(write_nbytes, write_bytes.size());
        CO_ASSERT_EQ(readed, write_bytes);
      }(client_.get(), server_.get()));
}

TEST_F(BufferTest, read_until) {
  TestRuntimeCtx::runtime()->block_on([](xyco::net::TcpStream *client, xyco::net::TcpStream *server)
                                          -> xyco::runtime::Future<void> {
    std::string_view write_bytes = "abc";
    auto write_nbytes = *co_await xyco::io::WriteExt::write(*client, write_bytes);
    *co_await client->shutdown(xyco::io::Shutdown::All);

    xyco::io::BufferReader<xyco::net::TcpStream, std::string> reader(server);
    auto line =
        *co_await xyco::io::BufferReadExt::read_until<decltype(reader), std::string>(reader, 'c');

    CO_ASSERT_EQ(write_nbytes, write_bytes.size());
    CO_ASSERT_EQ(line, write_bytes);
  }(client_.get(), server_.get()));
}

TEST_F(BufferTest, read_until_eof) {
  TestRuntimeCtx::runtime()->block_on([](xyco::net::TcpStream *client, xyco::net::TcpStream *server)
                                          -> xyco::runtime::Future<void> {
    std::string_view write_bytes = "ab";
    auto write_nbytes = *co_await xyco::io::WriteExt::write(*client, write_bytes);
    *co_await client->shutdown(xyco::io::Shutdown::All);

    xyco::io::BufferReader<xyco::net::TcpStream, std::string> reader(server);
    auto line =
        *co_await xyco::io::BufferReadExt::read_until<decltype(reader), std::string>(reader, 'c');

    CO_ASSERT_EQ(write_nbytes, write_bytes.size());
    std::string expected_line(write_bytes);
    expected_line.push_back('\0');
    CO_ASSERT_EQ(line, expected_line);
  }(client_.get(), server_.get()));
}

TEST_F(BufferTest, read_line) {
  TestRuntimeCtx::runtime()->block_on([](xyco::net::TcpStream *client, xyco::net::TcpStream *server)
                                          -> xyco::runtime::Future<void> {
    std::string write_bytes = "ab\nc";
    auto write_nbytes = *co_await xyco::io::WriteExt::write(*client, write_bytes);
    *co_await client->shutdown(xyco::io::Shutdown::All);

    xyco::io::BufferReader<xyco::net::TcpStream, std::string> reader(server);
    auto line = *co_await xyco::io::BufferReadExt::read_line<decltype(reader), std::string>(reader);

    CO_ASSERT_EQ(write_nbytes, write_bytes.size());
    CO_ASSERT_EQ(line, "ab\n");
  }(client_.get(), server_.get()));
}

TEST_F(BufferTest, buffer_read) {
  TestRuntimeCtx::runtime()->block_on(
      [](xyco::net::TcpStream *client,
         xyco::net::TcpStream *server) -> xyco::runtime::Future<void> {
        std::string_view write_bytes = "abcde";
        *co_await xyco::io::WriteExt::write(*client, write_bytes);
        *co_await client->shutdown(xyco::io::Shutdown::All);

        xyco::io::BufferReader<xyco::net::TcpStream, std::string> reader(server);
        auto readed = std::string(2, 0);
        auto read_bytes = *co_await reader.read(readed.begin(), readed.end());

        CO_ASSERT_EQ(read_bytes, readed.size());
        CO_ASSERT_EQ(readed, "ab");

        readed = std::string(3, 0);
        read_bytes = *co_await reader.read(readed.begin(), readed.end());

        CO_ASSERT_EQ(read_bytes, readed.size());
        CO_ASSERT_EQ(readed, "cde");
      }(client_.get(), server_.get()));
}

TEST_F(BufferTest, c_array_buffer) {
  TestRuntimeCtx::runtime()->block_on([](xyco::net::TcpStream *client, xyco::net::TcpStream *server)
                                          -> xyco::runtime::Future<void> {
    // NOLINTNEXTLINE(modernize-avoid-c-arrays)
    char write_bytes[] = "ab";
    *co_await xyco::io::WriteExt::write_all(*client, write_bytes);
    *co_await client->shutdown(xyco::io::Shutdown::All);

    // NOLINTNEXTLINE(modernize-avoid-c-arrays)
    char readed[4];
    *co_await xyco::io::ReadExt::read(*server, readed);
    CO_ASSERT_EQ(std::string(std::span(readed).data()), std::string(std::span(write_bytes).data()));
  }(client_.get(), server_.get()));
}
