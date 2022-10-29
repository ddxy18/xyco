#include "time/timeout.h"

#include <gtest/gtest.h>

#include "utils.h"

TEST(TimeoutTest, no_timeout) {
  constexpr std::chrono::milliseconds timeout_ms = std::chrono::milliseconds(3);

  TestRuntimeCtx::co_run(
      [&]() -> xyco::runtime::Future<void> {
        auto co_inner = []() -> xyco::runtime::Future<int> { co_return 1; };

        auto result = co_await xyco::time::timeout(timeout_ms, co_inner());

        CO_ASSERT_EQ(result.unwrap(), 1);
      },
      {timeout_ms + std::chrono::milliseconds(1)});
}

TEST(TimeoutTest, timeout) {
  constexpr std::chrono::milliseconds timeout_ms = std::chrono::milliseconds(3);

  TestRuntimeCtx::co_run(
      [&]() -> xyco::runtime::Future<void> {
        auto co_inner = [&]() -> xyco::runtime::Future<int> {
          co_await xyco::time::sleep(timeout_ms + std::chrono::milliseconds(2));
          co_return 1;
        };

        auto result = co_await xyco::time::timeout(timeout_ms, co_inner());

        CO_ASSERT_EQ(result.is_err(), true);
      },
      {timeout_ms + std::chrono::milliseconds(1)});
}

TEST(TimeoutTest, longer_timeout) {
  constexpr std::chrono::milliseconds timeout_ms =
      std::chrono::milliseconds(60);

  TestRuntimeCtx::co_run(
      [&]() -> xyco::runtime::Future<void> {
        auto co_inner = [&]() -> xyco::runtime::Future<int> {
          co_await xyco::time::sleep(timeout_ms);
          co_return 1;
        };

        auto result = co_await xyco::time::timeout(
            timeout_ms + +std::chrono::milliseconds(2), co_inner());

        CO_ASSERT_EQ(result.is_ok(), true);
      },
      {timeout_ms + std::chrono::milliseconds(1),
       std::chrono::milliseconds(2)});
}

TEST(TimeoutTest, void_future) {
  constexpr std::chrono::milliseconds timeout_ms = std::chrono::milliseconds(3);

  TestRuntimeCtx::co_run(
      [&]() -> xyco::runtime::Future<void> {
        auto co_inner = []() -> xyco::runtime::Future<void> { co_return; };

        auto result = co_await xyco::time::timeout(timeout_ms, co_inner());

        CO_ASSERT_EQ(result.is_ok(), true);
      },
      {timeout_ms + std::chrono::milliseconds(1)});
}
