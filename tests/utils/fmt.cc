#include <gtest/gtest.h>

#include <chrono>

#include "io/utils.h"
#include "net/socket.h"
#include "runtime/registry.h"

TEST(FmtTypeTest, IoError) {
  auto io_error = xyco::io::IoError();
  io_error.errno_ = EINTR;

  auto fmt_str = fmt::format("{}", io_error);

  ASSERT_EQ(fmt_str, "IoError{errno=4, info=}");
}

TEST(FmtTypeTest, IoExtra_Event) {
  auto event = xyco::runtime::Event{
      .extra_ = xyco::runtime::IoExtra{
          .state_ = xyco::runtime::IoExtra::State::All, .fd_ = 4}};

  auto fmt_str = fmt::format("{}", event);

  ASSERT_EQ(fmt_str, "Event{extra_=IoExtra{fd_=4, state_=All}}");
}

TEST(FmtTypeTest, TimeExtra_Event) {
  auto event = xyco::runtime::Event{
      .extra_ = xyco::runtime::TimeExtra{
          .expire_time_ = std::chrono::system_clock::time_point(
              std::chrono::milliseconds(1))}};

  auto fmt_str = fmt::format("{}", event);

  ASSERT_EQ(fmt_str,
            "Event{extra_=TimeExtra{expire_time_=1970-01-01 00:00:00}}");
}

TEST(FmtTypeTest, AsyncFutureExtra_Event) {
  auto event =
      xyco::runtime::Event{.extra_ = xyco::runtime::AsyncFutureExtra{}};

  auto fmt_str = fmt::format("{}", event);

  ASSERT_EQ(fmt_str, "Event{extra_=AsyncFutureExtra{}}");
}

TEST(FmtTypeTest, SocketAddr_ipv4) {
  const char *ip = "127.0.0.1";
  const uint16_t port = 80;

  auto local_http_addr = xyco::net::SocketAddr::new_v4(ip, port);

  auto fmt_str = fmt::format("{}", local_http_addr);

  ASSERT_EQ(fmt_str, "SocketAddr{ip=127.0.0.1,port=80}");
}

TEST(FmtTypeTest, SocketAddr_ipv6) {
  const char *ip = "0:0:0:0:0:0:0:1";
  const uint16_t port = 80;

  auto local_http_addr = xyco::net::SocketAddr::new_v6(ip, port);

  auto fmt_str = fmt::format("{}", local_http_addr);

  ASSERT_EQ(fmt_str, "SocketAddr{ip=::1,port=80}");
}

TEST(FmtTypeTest, Socket) {
  auto socket = xyco::net::Socket(-1);

  auto fmt_str = fmt::format("{}", socket);

  ASSERT_EQ(fmt_str, "Socket{fd_=-1}");
}