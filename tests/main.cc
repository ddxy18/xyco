#include <gtest/gtest.h>

#include "time/clock.h"
#include "utils.h"

auto main(int argc, char **argv) -> int {
  xyco::time::Clock::init<xyco::time::FrozenClock>();
  TestRuntimeCtx::init();

  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
