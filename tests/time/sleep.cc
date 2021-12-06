#include "time/sleep.h"

#include <gtest/gtest.h>

#include "utils.h"

TEST(SleepTest, sleep_accuracy) {
  TestRuntimeCtx::co_run([]() -> xyco::runtime::Future<void> {
    constexpr std::chrono::milliseconds timeout_ms =
        std::chrono::milliseconds(10);

    auto before_run = std::chrono::system_clock::now();
    co_await xyco::time::sleep(timeout_ms);
    auto interval = std::chrono::duration_cast<std::chrono::milliseconds>(
                        std::chrono::system_clock::now() - before_run)
                        .count();

    CO_ASSERT_EQ(interval >= (timeout_ms - time_deviation).count() &&
                     interval <= (timeout_ms + time_deviation).count(),
                 true);
  });
}