#include <gtest/gtest.h>

#include "io/driver.h"
#include "net/socket.h"
#include "runtime/blocking.h"
#include "time/driver.h"

TEST(FmtTypeTest, IoError) {
  auto io_error = xyco::utils::Error();
  io_error.errno_ = EINTR;

  auto fmt_str = fmt::format("{}", io_error);

  ASSERT_EQ(fmt_str, "IoError{errno=4, info=}");
}

TEST(FmtTypeTest, file_IoError) {
  auto io_error = xyco::utils::Error();
  io_error.errno_ = std::__to_underlying(xyco::utils::ErrorKind::Unsupported);

  auto fmt_str = fmt::format("{}", io_error);

  ASSERT_EQ(fmt_str, "IoError{error_kind=Unsupported, info=}");

  io_error.errno_ = std::__to_underlying(xyco::utils::ErrorKind::Uncategorized);
  fmt_str = fmt::format("{}", io_error);

  ASSERT_EQ(fmt_str, "IoError{error_kind=Uncategorized, info=}");
}

TEST(FmtTypeTest, IoExtra_Event) {
  auto event =
      xyco::runtime::Event{.extra_ = std::make_unique<xyco::io::IoExtra>(
                               xyco::io::IoExtra::Interest::All, 4)};
  auto *extra = dynamic_cast<xyco::io::IoExtra *>(event.extra_.get());

  extra->state_.set_field<xyco::io::IoExtra::State::Registered>();
  auto fmt_str = fmt::format("{}", event);
  ASSERT_EQ(fmt_str,
            "Event{extra_=IoExtra{state_=[Registered], interest_=All, "
            "fd_=4}}");

  extra->state_.set_field<xyco::io::IoExtra::State::Readable>();
  extra->state_.set_field<xyco::io::IoExtra::State::Writable>();
  fmt_str = fmt::format("{}", event);
  ASSERT_EQ(fmt_str,
            "Event{extra_=IoExtra{state_=[Registered,Readable,Writable], "
            "interest_=All, "
            "fd_=4}}");

  extra->state_.set_field<xyco::io::IoExtra::State::Readable, false>();
  extra->state_.set_field<xyco::io::IoExtra::State::Writable, false>();
  extra->state_.set_field<xyco::io::IoExtra::State::Pending>();
  fmt_str = fmt::format("{}", event);
  ASSERT_EQ(fmt_str,
            "Event{extra_=IoExtra{state_=[Registered,Pending], interest_=All, "
            "fd_=4}}");

  extra->state_.set_field<xyco::io::IoExtra::State::Pending, false>();
  extra->state_.set_field<xyco::io::IoExtra::State::Readable>();
  extra->interest_ = xyco::io::IoExtra::Interest::Read;
  fmt_str = fmt::format("{}", event);
  ASSERT_EQ(fmt_str,
            "Event{extra_=IoExtra{state_=[Registered,Readable], "
            "interest_=Read, fd_=4}}");

  extra->state_.set_field<xyco::io::IoExtra::State::Readable, false>();
  extra->state_.set_field<xyco::io::IoExtra::State::Writable>();
  extra->interest_ = xyco::io::IoExtra::Interest::Write;
  fmt_str = fmt::format("{}", event);
  ASSERT_EQ(fmt_str,
            "Event{extra_=IoExtra{state_=[Registered,Writable], "
            "interest_=Write, fd_=4}}");

  extra->state_.set_field<xyco::io::IoExtra::State::Writable, false>();
  extra->state_.set_field<xyco::io::IoExtra::State::Error>();
  fmt_str = fmt::format("{}", event);
  ASSERT_EQ(fmt_str,
            "Event{extra_=IoExtra{state_=[Registered,Error], interest_=Write, "
            "fd_=4}}");
}

TEST(FmtTypeTest, TimeExtra_Event) {
  auto event = xyco::runtime::Event{
      .extra_ = std::make_unique<xyco::time::TimeExtra>(
          std::chrono::system_clock::time_point(std::chrono::milliseconds(1)))};

  auto fmt_str = fmt::format("{}", event);

  ASSERT_EQ(fmt_str,
            "Event{extra_=TimeExtra{expire_time_=1970-01-01 00:00:00}}");
}

TEST(FmtTypeTest, AsyncFutureExtra_Event) {
  auto event = xyco::runtime::Event{
      .extra_ = std::make_unique<xyco::runtime::AsyncFutureExtra>([]() {})};

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