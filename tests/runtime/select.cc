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
  constexpr std::chrono::milliseconds timeout_ms = std::chrono::milliseconds(3);

  TestRuntimeCtx::co_run(
      [&]() -> xyco::runtime::Future<void> {
        auto common_value = -1;
        auto co1 = [&]() -> xyco::runtime::Future<int> {
          co_await xyco::time::sleep(2 * timeout_ms);

          common_value = 1;

          co_return 1;
        };

        auto co2 = [&]() -> xyco::runtime::Future<std::string> {
          co_await xyco::time::sleep(timeout_ms);

          common_value = 2;

          co_return "abc";
        };

        auto result = co_await xyco::runtime::select(co1(), co2());

        co_await xyco::time::sleep(timeout_ms + std::chrono::milliseconds(1));

        CO_ASSERT_EQ(std::get<1>(result).inner_, "abc");
        CO_ASSERT_EQ(common_value, 2);  // co1 is cancelled
      },
      {timeout_ms + std::chrono::milliseconds(1),
       timeout_ms + std::chrono::milliseconds(2)});
}

TEST(SelectTest, select_void) {
  TestRuntimeCtx::co_run([]() -> xyco::runtime::Future<void> {
    auto co1 = []() -> xyco::runtime::Future<void> { co_return; };

    auto co2 = []() -> xyco::runtime::Future<std::string> { co_return "abc"; };

    auto result = co_await xyco::runtime::select(co1(), co2());

    CO_ASSERT_EQ(result.index(), 0);
  });
}