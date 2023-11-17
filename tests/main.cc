#include <gtest/gtest.h>

#include "xyco/utils/logger.h"

import xyco.test.utils;
import xyco.time;

auto main(int argc, char **argv) -> int {
  xyco::time::Clock::init<xyco::time::FrozenClock>();
  TestRuntimeCtx::init();

  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
