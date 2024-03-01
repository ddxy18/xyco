#include <gtest/gtest.h>

#include <format>

import xyco.test.utils;
import xyco.runtime_core;
import xyco.io;

TEST(FmtTypeTest, IoExtra_Event) {
  auto event = xyco::runtime::Event{
      .extra_ = std::make_unique<xyco::io::IoExtra>(xyco::io::IoExtra::Interest::All, 4)};
  auto *extra = dynamic_cast<xyco::io::IoExtra *>(event.extra_.get());

  extra->state_.set_field<xyco::io::IoExtra::State::Registered>();
  auto fmt_str = std::format("{}", *extra);
  ASSERT_EQ(fmt_str, "IoExtra{state_=[Registered], interest_=All, fd_=4}");

  fmt_str = std::format("{}", event);
  ASSERT_EQ(fmt_str,
            "Event{extra_=IoExtra{state_=[Registered], interest_=All, "
            "fd_=4}}");

  extra->state_.set_field<xyco::io::IoExtra::State::Readable>();
  extra->state_.set_field<xyco::io::IoExtra::State::Writable>();
  fmt_str = std::format("{}", event);
  ASSERT_EQ(fmt_str,
            "Event{extra_=IoExtra{state_=[Registered,Readable,Writable], "
            "interest_=All, "
            "fd_=4}}");

  extra->state_.set_field<xyco::io::IoExtra::State::Readable, false>();
  extra->state_.set_field<xyco::io::IoExtra::State::Writable, false>();
  extra->state_.set_field<xyco::io::IoExtra::State::Pending>();
  fmt_str = std::format("{}", event);
  ASSERT_EQ(fmt_str,
            "Event{extra_=IoExtra{state_=[Registered,Pending], interest_=All, "
            "fd_=4}}");

  extra->state_.set_field<xyco::io::IoExtra::State::Pending, false>();
  extra->state_.set_field<xyco::io::IoExtra::State::Readable>();
  extra->interest_ = xyco::io::IoExtra::Interest::Read;
  fmt_str = std::format("{}", event);
  ASSERT_EQ(fmt_str,
            "Event{extra_=IoExtra{state_=[Registered,Readable], "
            "interest_=Read, fd_=4}}");

  extra->state_.set_field<xyco::io::IoExtra::State::Readable, false>();
  extra->state_.set_field<xyco::io::IoExtra::State::Writable>();
  extra->interest_ = xyco::io::IoExtra::Interest::Write;
  fmt_str = std::format("{}", event);
  ASSERT_EQ(fmt_str,
            "Event{extra_=IoExtra{state_=[Registered,Writable], "
            "interest_=Write, fd_=4}}");

  extra->state_.set_field<xyco::io::IoExtra::State::Writable, false>();
  extra->state_.set_field<xyco::io::IoExtra::State::Error>();
  fmt_str = std::format("{}", event);
  ASSERT_EQ(fmt_str,
            "Event{extra_=IoExtra{state_=[Registered,Error], interest_=Write, "
            "fd_=4}}");
}
