#include "time/timeout.h"

#include <gtest/gtest.h>

#include "utils.h"

TEST(TimeoutTest, no_timeout) {
  TestRuntimeCtx::co_run([]() -> xyco::runtime::Future<void> {
    constexpr std::chrono::milliseconds timeout_ms =
        std::chrono::milliseconds(10);

    auto co_inner = []() -> xyco::runtime::Future<int> { co_return 1; };

    auto result = co_await xyco::time::timeout(timeout_ms, co_inner());

    CO_ASSERT_EQ(result.unwrap(), 1);
  });
}

TEST(TimeoutTest, timeout) {
  TestRuntimeCtx::co_run([]() -> xyco::runtime::Future<void> {
    constexpr std::chrono::milliseconds timeout_ms =
        std::chrono::milliseconds(10);

    auto co_inner = [&]() -> xyco::runtime::Future<int> {
      co_await xyco::time::sleep(timeout_ms + time_deviation);
      co_return 1;
    };

    auto result = co_await xyco::time::timeout(timeout_ms, co_inner());

    CO_ASSERT_EQ(result.is_err(), true);
  });
}

TEST(TimeoutTest, longer_timeout) {
  TestRuntimeCtx::co_run([]() -> xyco::runtime::Future<void> {
    constexpr std::chrono::milliseconds timeout_ms =
        std::chrono::milliseconds(6);

    auto co_inner = [&]() -> xyco::runtime::Future<int> {
      co_await xyco::time::sleep(10 * timeout_ms);
      co_return 1;
    };

    auto result = co_await xyco::time::timeout(11 * timeout_ms, co_inner());

    CO_ASSERT_EQ(result.is_ok(), true);
  });
}

TEST(TimeoutTest, void_future) {
  TestRuntimeCtx::co_run([]() -> xyco::runtime::Future<void> {
    constexpr std::chrono::milliseconds timeout_ms =
        std::chrono::milliseconds(10);

    auto co_inner = []() -> xyco::runtime::Future<void> { co_return; };

    auto result = co_await xyco::time::timeout(timeout_ms, co_inner());

    CO_ASSERT_EQ(result.is_ok(), true);
  });
}