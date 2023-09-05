#include "xyco/time/timeout.h"

#include <gtest/gtest.h>

#include "utils.h"

TEST(TimeoutTest, no_timeout) {
  constexpr std::chrono::milliseconds timeout_ms = std::chrono::milliseconds(3);

  TestRuntimeCtx::co_run(
      [](const std::chrono::milliseconds timeout_ms)
          -> xyco::runtime::Future<void> {
        auto co_inner = []() -> xyco::runtime::Future<int> { co_return 1; };

        auto result = co_await xyco::time::timeout(timeout_ms, co_inner());

        CO_ASSERT_EQ(*result, 1);
      }(timeout_ms),
      {timeout_ms + std::chrono::milliseconds(1)});
}

TEST(TimeoutTest, timeout) {
  constexpr std::chrono::milliseconds timeout_ms = std::chrono::milliseconds(3);

  TestRuntimeCtx::co_run(
      [](const std::chrono::milliseconds timeout_ms)
          -> xyco::runtime::Future<void> {
        auto co_inner = [](const std::chrono::milliseconds timeout_ms)
            -> xyco::runtime::Future<int> {
          co_await xyco::time::sleep(timeout_ms + std::chrono::milliseconds(2));
          co_return 1;
        };

        auto result =
            co_await xyco::time::timeout(timeout_ms, co_inner(timeout_ms));

        CO_ASSERT_EQ(result.has_value(), false);
      }(timeout_ms),
      {timeout_ms + std::chrono::milliseconds(1)});
}

TEST(TimeoutTest, longer_timeout) {
  constexpr std::chrono::milliseconds timeout_ms =
      std::chrono::milliseconds(60);

  TestRuntimeCtx::co_run(
      [](const std::chrono::milliseconds timeout_ms)
          -> xyco::runtime::Future<void> {
        auto co_inner = [](const std::chrono::milliseconds timeout_ms)
            -> xyco::runtime::Future<int> {
          co_await xyco::time::sleep(timeout_ms);
          co_return 1;
        };

        auto result = co_await xyco::time::timeout(
            timeout_ms + +std::chrono::milliseconds(2), co_inner(timeout_ms));

        CO_ASSERT_EQ(result.has_value(), true);
      }(timeout_ms),
      {timeout_ms + std::chrono::milliseconds(1),
       std::chrono::milliseconds(2)});
}

TEST(TimeoutTest, void_future) {
  constexpr std::chrono::milliseconds timeout_ms = std::chrono::milliseconds(3);

  TestRuntimeCtx::co_run(
      [](const std::chrono::milliseconds timeout_ms)
          -> xyco::runtime::Future<void> {
        auto co_inner = []() -> xyco::runtime::Future<void> { co_return; };

        auto result = co_await xyco::time::timeout(timeout_ms, co_inner());

        CO_ASSERT_EQ(result.has_value(), true);
      }(timeout_ms),
      {timeout_ms + std::chrono::milliseconds(1)});
}
