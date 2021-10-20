#include "runtime/utils/select.h"

#include <gtest/gtest.h>

#include <chrono>
#include <string>

#include "runtime/runtime.h"
#include "time/sleep.h"
#include "utils.h"

TEST(SelectTest, select_immediate_ready) {
  TestRuntimeCtx::co_run({[]() -> xyco::runtime::Future<void> {
    auto co1 = []() -> xyco::runtime::Future<int> { co_return 1; };

    auto co2 = []() -> xyco::runtime::Future<std::string> { co_return "abc"; };

    auto result = co_await xyco::runtime::select(co1(), co2());

    CO_ASSERT_EQ(std::get<0>(result).inner_, 1);
  }});
}

TEST(SelectTest, select_delay) {
  TestRuntimeCtx::co_run({[]() -> xyco::runtime::Future<void> {
    auto co1 = []() -> xyco::runtime::Future<int> {
      constexpr auto sleep_ms = std::chrono::milliseconds(20);

      co_await xyco::time::sleep(sleep_ms);

      co_return 1;
    };

    auto co2 = []() -> xyco::runtime::Future<std::string> {
      constexpr auto sleep_ms = std::chrono::milliseconds(10);

      co_await xyco::time::sleep(sleep_ms);

      co_return "abc";
    };

    auto result = co_await xyco::runtime::select(co1(), co2());

    CO_ASSERT_EQ(std::get<1>(result).inner_, "abc");
  }});
}

TEST(SelectTest, select_void) {
  TestRuntimeCtx::co_run({[]() -> xyco::runtime::Future<void> {
    auto co1 = []() -> xyco::runtime::Future<void> { co_return; };

    auto co2 = []() -> xyco::runtime::Future<std::string> { co_return "abc"; };

    auto result = co_await xyco::runtime::select(co1(), co2());

    CO_ASSERT_EQ(result.index(), 0);
  }});
}