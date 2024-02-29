#include <gtest/gtest.h>

#include <coroutine>

import xyco.test.utils;
import xyco.task;

class JoinTest : public ::testing::Test {
 public:
  static auto null_co() -> xyco::runtime::Future<std::nullptr_t> { co_return nullptr; }

  static auto str_co() -> xyco::runtime::Future<std::string> { co_return "a"; }

  static auto int_co() -> xyco::runtime::Future<int> { co_return 1; }

  static auto failed_co() -> xyco::runtime::Future<int> {
    throw std::runtime_error("fail co");
    co_return 1;
  }

  static auto moveonly_co() -> xyco::runtime::Future<MoveOnlyObject> { co_return {}; }
};

TEST_F(JoinTest, join_moveonly) {
  auto [result] = TestRuntimeCtx::runtime()->block_on(xyco::task::join(moveonly_co()));

  CO_ASSERT_EQ(result, MoveOnlyObject());
}

TEST_F(JoinTest, join_null) {
  auto [single_result] = TestRuntimeCtx::runtime()->block_on(xyco::task::join(null_co()));

  ASSERT_EQ(single_result, nullptr);
}

TEST_F(JoinTest, join_empty) {
  [[maybe_unused]] auto empty_tuple = TestRuntimeCtx::runtime()->block_on(xyco::task::join());

  ASSERT_EQ(std::tuple_size_v<decltype(empty_tuple)>, 0);
}

TEST_F(JoinTest, join_one) {
  auto [single_result] = TestRuntimeCtx::runtime()->block_on(xyco::task::join(int_co()));

  ASSERT_EQ(single_result, 1);
}

TEST_F(JoinTest, join_two) {
  auto [first, second] = TestRuntimeCtx::runtime()->block_on(xyco::task::join(null_co(), str_co()));

  ASSERT_EQ(first, nullptr);
  ASSERT_EQ(second, "a");
}

TEST_F(JoinTest, join_three) {
  auto [first, second, third] =
      TestRuntimeCtx::runtime()->block_on(xyco::task::join(null_co(), str_co(), int_co()));

  ASSERT_EQ(first, nullptr);
  ASSERT_EQ(second, "a");
  ASSERT_EQ(third, 1);
}

TEST_F(JoinTest, join_partially_failed) {
  EXPECT_THROW({ TestRuntimeCtx::runtime()->block_on(xyco::task::join(null_co(), failed_co())); },
               std::runtime_error);
}
