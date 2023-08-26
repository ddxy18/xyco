#include <gtest/gtest.h>

#include "utils.h"

TEST(RuntimeTest, block_on_void) {
  TestRuntimeCtx::runtime()->block_on(
      []() -> xyco::runtime::Future<void> { co_return; }());
}

TEST(RuntimeTest, block_on_with_return_value) {
  auto result = TestRuntimeCtx::runtime()->block_on(
      []() -> xyco::runtime::Future<int> { co_return 1; }());
  ASSERT_EQ(result, 1);
}
