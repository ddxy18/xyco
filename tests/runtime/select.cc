#include "xyco/task/select.h"

#include <gtest/gtest.h>

#include "utils.h"
#include "xyco/time/sleep.h"

class SelectTest : public ::testing::Test {
 public:
  static auto int_co() -> xyco::runtime::Future<int> { co_return 1; }

  static auto str_co() -> xyco::runtime::Future<std::string> { co_return "a"; }

  static auto null_co() -> xyco::runtime::Future<std::nullptr_t> {
    co_return nullptr;
  }

  static auto failed_co() -> xyco::runtime::Future<int> {
    throw std::runtime_error("fail co");
    co_return 1;
  }
};

TEST_F(SelectTest, select_null) {
  auto [result] =
      TestRuntimeCtx::runtime()->block_on(xyco::task::select(null_co()));

  CO_ASSERT_EQ(result.value(), nullptr);
}

TEST_F(SelectTest, select_empty) {
  auto result = TestRuntimeCtx::runtime()->block_on(xyco::task::select());

  CO_ASSERT_EQ(std::tuple_size<decltype(result)>(), 0);
}

TEST_F(SelectTest, select_one) {
  auto [result] =
      TestRuntimeCtx::runtime()->block_on(xyco::task::select(int_co()));

  CO_ASSERT_EQ(result.value(), 1);
}

TEST_F(SelectTest, select_two) {
  auto [result_first, result_second] = TestRuntimeCtx::runtime()->block_on(
      xyco::task::select(int_co(), str_co()));

  CO_ASSERT_EQ(result_first.value(), 1);
}

TEST_F(SelectTest, select_three) {
  auto [result_first, result_second, result_third] =
      TestRuntimeCtx::runtime()->block_on(
          xyco::task::select(int_co(), str_co(), null_co()));

  CO_ASSERT_EQ(result_first.value(), 1);
}

TEST_F(SelectTest, select_throw_exception) {
  EXPECT_THROW(
      { TestRuntimeCtx::runtime()->block_on(xyco::task::select(failed_co())); },
      std::runtime_error);
}

TEST_F(SelectTest, select_delay) {
  constexpr std::chrono::milliseconds timeout_ms = std::chrono::milliseconds(3);

  TestRuntimeCtx::co_run(
      [&]() -> xyco::runtime::Future<void> {
        auto common_value = -1;
        auto co1 = [&]() -> xyco::runtime::Future<int> {
          co_await xyco::time::sleep(2 * timeout_ms);

          common_value = 1;

          co_return 1;
        };

        auto co2 = [&]() -> xyco::runtime::Future<std::string> {
          co_await xyco::time::sleep(timeout_ms);

          common_value = 2;

          co_return "abc";
        };

        auto [result_first, result_second] =
            co_await xyco::task::select(co1(), co2());

        co_await xyco::time::sleep(timeout_ms + std::chrono::milliseconds(1));

        CO_ASSERT_EQ(result_second.value(), "abc");
        CO_ASSERT_EQ(common_value, 2);  // co1 is cancelled
      },
      {timeout_ms + std::chrono::milliseconds(1),
       timeout_ms + std::chrono::milliseconds(2)});
}
