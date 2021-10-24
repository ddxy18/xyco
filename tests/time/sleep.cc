#include "time/sleep.h"

#include <gtest/gtest.h>

#include "utils.h"

TEST(SleepTest, sleep_accuracy) {
  TestRuntimeCtx::co_run([]() -> xyco::runtime::Future<void> {
    constexpr std::chrono::milliseconds timeout_ms =
        std::chrono::milliseconds(10);

    auto before_run = std::chrono::system_clock::now();
    co_await xyco::time::sleep(timeout_ms);
    auto interval = (std::chrono::system_clock::now() - before_run).count();

    CO_ASSERT_EQ(
        interval >=
            std::chrono::duration_cast<std::chrono::system_clock::duration>(
                timeout_ms)
                .count(),
        true);
    CO_ASSERT_EQ(
        interval <
            std::chrono::duration_cast<std::chrono::system_clock::duration>(
                timeout_ms + std::chrono::milliseconds(3))
                .count(),
        true);
  });
}