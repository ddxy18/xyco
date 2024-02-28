#include <gtest/gtest.h>

#include <format>

import xyco.test.utils;
import xyco.runtime_core;
import xyco.libc;
import xyco.io;

TEST(FmtTypeTest, IoExtra_Event) {
  auto event =
      xyco::runtime::Event{.extra_ = std::make_unique<xyco::io::IoExtra>()};
  auto *extra = dynamic_cast<xyco::io::IoExtra *>(event.extra_.get());

  extra->fd_ = 1;
  extra->args_ = xyco::io::IoExtra::Read{.len_ = 1, .offset_ = 0};
  auto fmt_str = std::format("{}", *extra);
  ASSERT_EQ(fmt_str,
            "IoExtra{args_=Read{len_=1, offset_=0}, fd_=1, return_=0}");

  fmt_str = std::format("{}", event);
  ASSERT_EQ(
      fmt_str,
      "Event{extra_=IoExtra{args_=Read{len_=1, offset_=0}, fd_=1, return_=0}}");

  extra->args_ = xyco::io::IoExtra::Write{.len_ = 1, .offset_ = 0};
  fmt_str = std::format("{}", event);
  ASSERT_EQ(fmt_str,
            "Event{extra_=IoExtra{args_=Write{len_=1, offset_=0}, fd_=1, "
            "return_=0}}");

  extra->args_ = xyco::io::IoExtra::Close{};
  fmt_str = std::format("{}", event);
  ASSERT_EQ(fmt_str, "Event{extra_=IoExtra{args_=Close{}, fd_=1, return_=0}}");

  constexpr auto port = 8888;
  xyco::libc::in_addr char_addr{};
  xyco::libc::sockaddr_in addr{};
  xyco::libc::socklen_t len = 0;
  xyco::libc::inet_pton(xyco::libc::K_AF_INET, "127.0.0.1", &char_addr);
  addr.sin_family = xyco::libc::K_AF_INET;
  addr.sin_addr = char_addr;
  addr.sin_port = port;
  extra->args_ = xyco::io::IoExtra::Accept{
      // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
      .addr_ = reinterpret_cast<xyco::libc::sockaddr *>(&addr),
      .addrlen_ = &len,
      .flags_ = 0};
  fmt_str = std::format("{}", event);
  ASSERT_EQ(fmt_str,
            "Event{extra_=IoExtra{args_=Accept{addr_={127.0.0.1:8888}, "
            "flags_=0}, fd_=1, return_=0}}");

  xyco::libc::inet_pton(xyco::libc::K_AF_INET, "127.0.0.1", &char_addr);
  addr.sin_family = xyco::libc::K_AF_INET;
  addr.sin_addr = char_addr;
  addr.sin_port = port;
  extra->args_ = xyco::io::IoExtra::Connect{
      // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
      .addr_ = reinterpret_cast<xyco::libc::sockaddr *>(&addr),
      .addrlen_ = sizeof(addr)};
  fmt_str = std::format("{}", event);
  ASSERT_EQ(
      fmt_str,
      "Event{extra_=IoExtra{args_=Connect{addr_={127.0.0.1:8888}}, fd_=1, "
      "return_=0}}");

  extra->args_ =
      xyco::io::IoExtra::Shutdown{.shutdown_ = xyco::io::Shutdown::Read};
  fmt_str = std::format("{}", event);
  ASSERT_EQ(fmt_str,
            "Event{extra_=IoExtra{args_=Shutdown{shutdown_=Shutdown{Read}}, "
            "fd_=1, return_=0}}");

  extra->args_ =
      xyco::io::IoExtra::Shutdown{.shutdown_ = xyco::io::Shutdown::Write};
  fmt_str = std::format("{}", event);
  ASSERT_EQ(fmt_str,
            "Event{extra_=IoExtra{args_=Shutdown{shutdown_=Shutdown{Write}}, "
            "fd_=1, return_=0}}");

  extra->args_ =
      xyco::io::IoExtra::Shutdown{.shutdown_ = xyco::io::Shutdown::All};
  fmt_str = std::format("{}", event);
  ASSERT_EQ(fmt_str,
            "Event{extra_=IoExtra{args_=Shutdown{shutdown_=Shutdown{All}}, "
            "fd_=1, return_=0}}");
}
