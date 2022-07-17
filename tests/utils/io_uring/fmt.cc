#include <gtest/gtest.h>

#include "net/socket.h"
#include "runtime/blocking.h"
#include "time/driver.h"
#include "utils.h"

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
  auto event = xyco::runtime::Event{.extra_ = std::make_unique<io::IoExtra>()};
  auto *extra = dynamic_cast<io::IoExtra *>(event.extra_.get());

  extra->fd_ = 1;
  extra->args_ = io::IoExtra::Read{.len_ = 1, .offset_ = 0};
  auto fmt_str = fmt::format("{}", *extra);
  ASSERT_EQ(fmt_str,
            "IoExtra{args_=Read{len_=1, offset_=0}, fd_=1, return_=0}");

  fmt_str = fmt::format("{}", event);
  ASSERT_EQ(
      fmt_str,
      "Event{extra_=IoExtra{args_=Read{len_=1, offset_=0}, fd_=1, return_=0}}");

  extra->args_ = io::IoExtra::Write{.len_ = 1, .offset_ = 0};
  fmt_str = fmt::format("{}", event);
  ASSERT_EQ(fmt_str,
            "Event{extra_=IoExtra{args_=Write{len_=1, offset_=0}, fd_=1, "
            "return_=0}}");

  extra->args_ = io::IoExtra::Close{};
  fmt_str = fmt::format("{}", event);
  ASSERT_EQ(fmt_str, "Event{extra_=IoExtra{args_=Close{}, fd_=1, return_=0}}");

  in_addr char_addr{};
  sockaddr_in addr{};
  socklen_t len = 0;
  ::inet_pton(AF_INET, "127.0.0.1", &char_addr);
  addr.sin_family = AF_INET;
  addr.sin_addr = char_addr;
  addr.sin_port = 8888;
  extra->args_ = io::IoExtra::Accept{
      .addr_ = static_cast<sockaddr *>(static_cast<void *>(&addr)),
      .addrlen_ = &len,
      .flags_ = 0};
  fmt_str = fmt::format("{}", event);
  ASSERT_EQ(fmt_str,
            "Event{extra_=IoExtra{args_=Accept{addr_={127.0.0.1:8888}, "
            "flags_=0}, fd_=1, return_=0}}");

  ::inet_pton(AF_INET, "127.0.0.1", &char_addr);
  addr.sin_family = AF_INET;
  addr.sin_addr = char_addr;
  addr.sin_port = 8888;
  extra->args_ = io::IoExtra::Connect{
      .addr_ = static_cast<sockaddr *>(static_cast<void *>(&addr)),
      .addrlen_ = sizeof(addr)};
  fmt_str = fmt::format("{}", event);
  ASSERT_EQ(
      fmt_str,
      "Event{extra_=IoExtra{args_=Connect{addr_={127.0.0.1:8888}}, fd_=1, "
      "return_=0}}");

  extra->args_ = io::IoExtra::Shutdown{.shutdown_ = xyco::io::Shutdown::Read};
  fmt_str = fmt::format("{}", event);
  ASSERT_EQ(fmt_str,
            "Event{extra_=IoExtra{args_=Shutdown{shutdown_=Shutdown{Read}}, "
            "fd_=1, return_=0}}");

  extra->args_ = io::IoExtra::Shutdown{.shutdown_ = xyco::io::Shutdown::Write};
  fmt_str = fmt::format("{}", event);
  ASSERT_EQ(fmt_str,
            "Event{extra_=IoExtra{args_=Shutdown{shutdown_=Shutdown{Write}}, "
            "fd_=1, return_=0}}");

  extra->args_ = io::IoExtra::Shutdown{.shutdown_ = xyco::io::Shutdown::All};
  fmt_str = fmt::format("{}", event);
  ASSERT_EQ(fmt_str,
            "Event{extra_=IoExtra{args_=Shutdown{shutdown_=Shutdown{All}}, "
            "fd_=1, return_=0}}");
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