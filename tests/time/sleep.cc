#include "xyco/time/sleep.h"

#include <gtest/gtest.h>

#include "utils.h"

TEST(SleepTest, sleep_accuracy) {
  constexpr std::chrono::milliseconds timeout_ms = std::chrono::milliseconds(3);

  TestRuntimeCtx::co_run(
      [](const std::chrono::milliseconds timeout_ms)
          -> xyco::runtime::Future<void> {
        auto before_run = xyco::time::Clock::now();

        co_await xyco::time::sleep(timeout_ms);
        auto interval = std::chrono::duration_cast<std::chrono::milliseconds>(
                            xyco::time::Clock::now() - before_run)
                            .count();

        CO_ASSERT_EQ(
            interval >= (timeout_ms - std::chrono::milliseconds(1)).count() &&
                interval <= (timeout_ms + std::chrono::milliseconds(1)).count(),
            true);
      }(timeout_ms),
      {timeout_ms + std::chrono::milliseconds(1)});
}
