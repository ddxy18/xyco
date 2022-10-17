#include <gtest/gtest.h>

#include "utils.h"

TEST(FmtTypeTest, IoExtra_Event) {
  auto event =
      xyco::runtime::Event{.extra_ = std::make_unique<xyco::io::IoExtra>()};
  auto *extra = dynamic_cast<xyco::io::IoExtra *>(event.extra_.get());

  extra->fd_ = 1;
  extra->args_ = xyco::io::IoExtra::Read{.len_ = 1, .offset_ = 0};
  auto fmt_str = fmt::format("{}", *extra);
  ASSERT_EQ(fmt_str,
            "IoExtra{args_=Read{len_=1, offset_=0}, fd_=1, return_=0}");

  fmt_str = fmt::format("{}", event);
  ASSERT_EQ(
      fmt_str,
      "Event{extra_=IoExtra{args_=Read{len_=1, offset_=0}, fd_=1, return_=0}}");

  extra->args_ = xyco::io::IoExtra::Write{.len_ = 1, .offset_ = 0};
  fmt_str = fmt::format("{}", event);
  ASSERT_EQ(fmt_str,
            "Event{extra_=IoExtra{args_=Write{len_=1, offset_=0}, fd_=1, "
            "return_=0}}");

  extra->args_ = xyco::io::IoExtra::Close{};
  fmt_str = fmt::format("{}", event);
  ASSERT_EQ(fmt_str, "Event{extra_=IoExtra{args_=Close{}, fd_=1, return_=0}}");

  in_addr char_addr{};
  sockaddr_in addr{};
  socklen_t len = 0;
  ::inet_pton(AF_INET, "127.0.0.1", &char_addr);
  addr.sin_family = AF_INET;
  addr.sin_addr = char_addr;
  addr.sin_port = 8888;
  extra->args_ = xyco::io::IoExtra::Accept{
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
  extra->args_ = xyco::io::IoExtra::Connect{
      .addr_ = static_cast<sockaddr *>(static_cast<void *>(&addr)),
      .addrlen_ = sizeof(addr)};
  fmt_str = fmt::format("{}", event);
  ASSERT_EQ(
      fmt_str,
      "Event{extra_=IoExtra{args_=Connect{addr_={127.0.0.1:8888}}, fd_=1, "
      "return_=0}}");

  extra->args_ =
      xyco::io::IoExtra::Shutdown{.shutdown_ = xyco::io::Shutdown::Read};
  fmt_str = fmt::format("{}", event);
  ASSERT_EQ(fmt_str,
            "Event{extra_=IoExtra{args_=Shutdown{shutdown_=Shutdown{Read}}, "
            "fd_=1, return_=0}}");

  extra->args_ =
      xyco::io::IoExtra::Shutdown{.shutdown_ = xyco::io::Shutdown::Write};
  fmt_str = fmt::format("{}", event);
  ASSERT_EQ(fmt_str,
            "Event{extra_=IoExtra{args_=Shutdown{shutdown_=Shutdown{Write}}, "
            "fd_=1, return_=0}}");

  extra->args_ =
      xyco::io::IoExtra::Shutdown{.shutdown_ = xyco::io::Shutdown::All};
  fmt_str = fmt::format("{}", event);
  ASSERT_EQ(fmt_str,
            "Event{extra_=IoExtra{args_=Shutdown{shutdown_=Shutdown{All}}, "
            "fd_=1, return_=0}}");
}
