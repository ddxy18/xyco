#include "runtime/utils/join.h"

#include <gtest/gtest.h>

#include "time/sleep.h"
#include "utils.h"

TEST(JoinTest, join_immediate_ready) {
  TestRuntimeCtx::co_run([]() -> xyco::runtime::Future<void> {
    auto co1 = []() -> xyco::runtime::Future<int> { co_return 1; };

    auto co2 = []() -> xyco::runtime::Future<std::string> { co_return "abc"; };

    auto result = co_await xyco::runtime::join(co1(), co2());

    CO_ASSERT_EQ(result.first.inner_, 1);
    CO_ASSERT_EQ(result.second.inner_, "abc");
  });
}

TEST(JoinTest, join_delay) {
  TestRuntimeCtx::co_run([]() -> xyco::runtime::Future<void> {
    auto co1 = []() -> xyco::runtime::Future<int> {
      co_await xyco::time::sleep(2 * time_deviation);

      co_return 1;
    };

    auto co2 = []() -> xyco::runtime::Future<std::string> {
      co_await xyco::time::sleep(time_deviation);

      co_return "abc";
    };

    auto result = co_await xyco::runtime::join(co1(), co2());

    CO_ASSERT_EQ(result.first.inner_, 1);
    CO_ASSERT_EQ(result.second.inner_, "abc");
  });
}

TEST(JoinTest, join_void) {
  TestRuntimeCtx::co_run([]() -> xyco::runtime::Future<void> {
    auto co1 = []() -> xyco::runtime::Future<void> { co_return; };

    auto co2 = []() -> xyco::runtime::Future<std::string> { co_return "abc"; };

    auto result = co_await xyco::runtime::join(co1(), co2());

    CO_ASSERT_EQ(result.second.inner_, "abc");
  });
}