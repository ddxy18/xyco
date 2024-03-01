#include <gtest/gtest.h>

#include <coroutine>

import xyco.test.utils;
import xyco.task;

TEST(BlockingTaskTest, moveonly) {
  auto result = TestRuntimeCtx::runtime()->block_on([]() -> xyco::runtime::Future<MoveOnlyObject> {
    co_return co_await xyco::task::BlockingTask([]() { return MoveOnlyObject(); });
  }());
  ASSERT_EQ(result, MoveOnlyObject());
}
