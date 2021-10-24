#include "runtime/utils/select.h"

#include <gtest/gtest.h>

#include "time/sleep.h"
#include "utils.h"

TEST(SelectTest, select_immediate_ready) {
  TestRuntimeCtx::co_run([]() -> xyco::runtime::Future<void> {
    auto co1 = []() -> xyco::runtime::Future<int> { co_return 1; };

    auto co2 = []() -> xyco::runtime::Future<std::string> { co_return "abc"; };

    auto result = co_await xyco::runtime::select(co1(), co2());

    CO_ASSERT_EQ(std::get<0>(result).inner_, 1);
  });
}

TEST(SelectTest, select_delay) {
  TestRuntimeCtx::co_run([]() -> xyco::runtime::Future<void> {
    constexpr auto sleep_ms = std::chrono::milliseconds(10);

    auto common_value = -1;
    auto co1 = [&]() -> xyco::runtime::Future<int> {
      co_await xyco::time::sleep(sleep_ms + sleep_ms);

      common_value = 1;

      co_return 1;
    };

    auto co2 = [&]() -> xyco::runtime::Future<std::string> {
      co_await xyco::time::sleep(sleep_ms);

      common_value = 2;

      co_return "abc";
    };

    auto result = co_await xyco::runtime::select(co1(), co2());

    co_await xyco::time::sleep(std::chrono::milliseconds(sleep_ms + sleep_ms));

    CO_ASSERT_EQ(std::get<1>(result).inner_, "abc");
    CO_ASSERT_EQ(common_value, 2);  // co1 is cancelled
  });
}

TEST(SelectTest, select_void) {
  TestRuntimeCtx::co_run([]() -> xyco::runtime::Future<void> {
    auto co1 = []() -> xyco::runtime::Future<void> { co_return; };

    auto co2 = []() -> xyco::runtime::Future<std::string> { co_return "abc"; };

    auto result = co_await xyco::runtime::select(co1(), co2());

    CO_ASSERT_EQ(result.index(), 0);
  });
}