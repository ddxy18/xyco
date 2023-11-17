#include <gtest/gtest.h>

import xyco.test.utils;
import xyco.time;

// Uses death test here to ensure they run before other time related unit tests
// to avoid disordering the current time point.

TEST(ClockDeathTest, system_clock) {
  EXPECT_EXIT(
      {
        xyco::time::Clock::init<std::chrono::system_clock>();
        auto now = xyco::time::Clock::now();
        auto sys_now = std::chrono::system_clock::now();
        auto gap = std::chrono::duration_cast<std::chrono::milliseconds>(
            sys_now - now);

        ASSERT_EQ(gap.count(), 0);

        std::quick_exit(0);
      },
      testing::ExitedWithCode(0), "");
}

TEST(ClockDeathTest, steady_clock) {
  EXPECT_EXIT(
      {
        xyco::time::Clock::init<std::chrono::steady_clock>();
        auto now = xyco::time::Clock::now();
        auto steady_now = std::chrono::steady_clock::now();
        auto sys_now = std::chrono::system_clock::now();
        auto gap = std::chrono::duration_cast<std::chrono::milliseconds>(
            steady_now -
            std::chrono::time_point_cast<std::chrono::steady_clock::duration>(
                now - sys_now + steady_now));

        ASSERT_EQ(gap.count(), 0);

        std::quick_exit(0);
      },
      testing::ExitedWithCode(0), "");
}

TEST(ClockDeathTest, frozen_clock) {
  EXPECT_EXIT(
      {
        xyco::time::Clock::init<xyco::time::FrozenClock>();
        auto now = xyco::time::Clock::now();
        xyco::time::FrozenClock::advance(std::chrono::milliseconds(1));
        auto advance_now = xyco::time::Clock::now();
        auto gap = std::chrono::duration_cast<std::chrono::milliseconds>(
            advance_now - now);

        ASSERT_EQ(gap.count(), 1);

        std::quick_exit(0);
      },
      testing::ExitedWithCode(0), "");
}
