#include <gtest/gtest.h>

#include <coroutine>
#include <format>

import xyco.test.utils;
import xyco.runtime_core;
import xyco.time;
import xyco.net;
import xyco.task;
import xyco.fs;

TEST(FmtTypeTest, IoError) {
  auto io_error = xyco::utils::Error();
  io_error.errno_ = EINTR;

  auto fmt_str = std::format("{}", io_error);

  ASSERT_EQ(fmt_str, "IoError{errno=4, info=}");
}

TEST(FmtTypeTest, file_IoError) {
  auto io_error = xyco::utils::Error();
  io_error.errno_ = std::to_underlying(xyco::utils::ErrorKind::Unsupported);

  auto fmt_str = std::format("{}", io_error);

  ASSERT_EQ(fmt_str, "IoError{error_kind=Unsupported, info=}");

  io_error.errno_ = std::to_underlying(xyco::utils::ErrorKind::Uncategorized);
  fmt_str = std::format("{}", io_error);

  ASSERT_EQ(fmt_str, "IoError{error_kind=Uncategorized, info=}");
}

TEST(FmtTypeTest, TimeExtra_Event) {
  auto event = xyco::runtime::Event{
      .extra_ = std::make_unique<xyco::time::TimeExtra>(
          std::chrono::system_clock::time_point(std::chrono::milliseconds(1)))};

  auto fmt_str = std::format("{}", event);

  ASSERT_EQ(fmt_str,
            "Event{extra_=TimeExtra{expire_time_=1970-01-01 00:00:00}}");
}

TEST(FmtTypeTest, AsyncFutureExtra_Event) {
  auto event = xyco::runtime::Event{
      .extra_ = std::make_unique<xyco::task::BlockingExtra>([]() {})};

  auto fmt_str = std::format("{}", event);

  ASSERT_EQ(fmt_str, "Event{extra_=BlockingExtra{}}");
}

TEST(FmtTypeTest, SocketAddr_ipv4) {
  const char *ip_addr = "127.0.0.1";
  const uint16_t port = 80;

  auto local_http_addr = xyco::net::SocketAddr::new_v4(ip_addr, port);

  auto fmt_str = std::format("{}", local_http_addr);

  ASSERT_EQ(fmt_str, "SocketAddr{ip=127.0.0.1,port=80}");
}

TEST(FmtTypeTest, SocketAddr_ipv6) {
  const char *ip_addr = "0:0:0:0:0:0:0:1";
  const uint16_t port = 80;

  auto local_http_addr = xyco::net::SocketAddr::new_v6(ip_addr, port);

  auto fmt_str = std::format("{}", local_http_addr);

  ASSERT_EQ(fmt_str, "SocketAddr{ip=::1,port=80}");
}

TEST(FmtTypeTest, Socket) {
  auto socket = xyco::net::Socket(-1);

  auto fmt_str = std::format("{}", socket);

  ASSERT_EQ(fmt_str, "Socket{fd_=-1}");
}

TEST(FmtTypeTest, File) {
  TestRuntimeCtx::runtime()->block_on([]() -> xyco::runtime::Future<void> {
    auto file = *co_await xyco::fs::File ::open("README.md");

    auto fmt_str = std::format("{}", file);

    CO_ASSERT_EQ(fmt_str, "File{path_=README.md}");
  }());
}
