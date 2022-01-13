#include <gtest/gtest.h>

#include "io/driver.h"
#include "net/socket.h"
#include "runtime/blocking.h"
#include "time/driver.h"

TEST(FmtTypeTest, IoError) {
  auto io_error = xyco::io::IoError();
  io_error.errno_ = EINTR;

  auto fmt_str = fmt::format("{}", io_error);

  ASSERT_EQ(fmt_str, "IoError{errno=4, info=}");
}

TEST(FmtTypeTest, file_IoError) {
  auto io_error = xyco::io::IoError();
  io_error.errno_ = std::__to_underlying(xyco::io::ErrorKind::Unsupported);

  auto fmt_str = fmt::format("{}", io_error);

  ASSERT_EQ(fmt_str, "IoError{error_kind=Unsupported, info=}");

  io_error.errno_ = std::__to_underlying(xyco::io::ErrorKind::Uncategorized);
  fmt_str = fmt::format("{}", io_error);

  ASSERT_EQ(fmt_str, "IoError{error_kind=Uncategorized, info=}");
}

TEST(FmtTypeTest, IoExtra_Event) {
  auto extra = xyco::io::IoExtra(xyco::io::IoExtra::Interest::All, 4,
                                 xyco::io::IoExtra::State::All);
  auto event = xyco::runtime::Event{.extra_ = &extra};

  auto fmt_str = fmt::format("{}", event);
  ASSERT_EQ(fmt_str, "Event{extra_=IoExtra{state_=All, interest_=All, fd_=4}}");

  extra.state_ = xyco::io::IoExtra::State::Pending;
  fmt_str = fmt::format("{}", event);
  ASSERT_EQ(fmt_str,
            "Event{extra_=IoExtra{state_=Pending, interest_=All, fd_=4}}");

  extra.interest_ = xyco::io::IoExtra::Interest::Read;
  extra.state_ = xyco::io::IoExtra::State::Readable;
  fmt_str = fmt::format("{}", event);
  ASSERT_EQ(fmt_str,
            "Event{extra_=IoExtra{state_=Readable, interest_=Read, fd_=4}}");

  extra.interest_ = xyco::io::IoExtra::Interest::Write;
  extra.state_ = xyco::io::IoExtra::State::Writable;
  fmt_str = fmt::format("{}", event);
  ASSERT_EQ(fmt_str,
            "Event{extra_=IoExtra{state_=Writable, interest_=Write, fd_=4}}");
}

TEST(FmtTypeTest, TimeExtra_Event) {
  auto extra = xyco::time::TimeExtra(
      std::chrono::system_clock::time_point(std::chrono::milliseconds(1)));
  auto event = xyco::runtime::Event{.extra_ = &extra};

  auto fmt_str = fmt::format("{}", event);

  ASSERT_EQ(fmt_str,
            "Event{extra_=TimeExtra{expire_time_=1970-01-01 00:00:00}}");
}

TEST(FmtTypeTest, AsyncFutureExtra_Event) {
  auto extra = xyco::runtime::AsyncFutureExtra([]() {});
  auto event = xyco::runtime::Event{.extra_ = &extra};

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