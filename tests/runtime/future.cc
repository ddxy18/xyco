#include <gtest/gtest.h>

#include <gsl/pointers>

#include "utils.h"

TEST(InRuntimeDeathTest, NoSuspend) {
  TestRuntimeCtx::runtime()->block_on([]() -> xyco::runtime::Future<void> {
    int co_result = -1;
    auto co_innner = []() -> xyco::runtime::Future<int> { co_return 1; };
    co_result = co_await co_innner();

    CO_ASSERT_EQ(co_result, 1);
  }());
}

class SuspendOnceFuture : public xyco::runtime::Future<int> {
 public:
  SuspendOnceFuture(xyco::runtime::Future<int> *&handle)
      : xyco::runtime::Future<int>(nullptr), handle_(handle) {}

  [[nodiscard]] auto poll([[maybe_unused]] xyco::runtime::Handle<void> self)
      -> xyco::runtime::Poll<int> override {
    if (!ready_) {
      ready_ = true;
      handle_ = this;
      return xyco::runtime::Pending();
    }
    return xyco::runtime::Ready<int>{-1};
  }

 private:
  bool ready_{};
  // NOLINTNEXTLINE(cppcoreguidelines-avoid-const-or-ref-data-members)
  xyco::runtime::Future<int> *&handle_;
};

class UpdateEvaluator {
 public:
  // NOLINTNEXTLINE(cppcoreguidelines-avoid-reference-coroutine-parameters,cppcoreguidelines-rvalue-reference-param-not-moved)
  auto update(xyco::runtime::Future<int> &&async_update)
      -> xyco::runtime::Future<void> {
    co_result = co_await async_update;
  };

  int co_result{0};
};

TEST(InRuntimeDeathTest, Suspend) {
  xyco::runtime::Future<int> *handle = nullptr;

  auto evaluator = UpdateEvaluator();
  auto suspend_once = evaluator.update(SuspendOnceFuture(handle));

  EXPECT_DEATH(
      {
        suspend_once.get_handle().resume();
        handle->get_handle().resume();

        std::quick_exit(evaluator.co_result);
      },
      "");
}

TEST(InRuntimeDeathTest, NoSuspend_loop) {
  constexpr int ITERATION_TIMES = 100000 / 8;

  TestRuntimeCtx::runtime()->block_on([]() -> xyco::runtime::Future<void> {
    auto co_innner = []() -> xyco::runtime::Future<int> { co_return 1; };

    // NOTE: Iteration times should be restricted carefully in this no suspend
    // loop case to avoid stack overflow.
    for (int i = 0; i < ITERATION_TIMES; i++) {
      int co_result = -1;
      co_result = co_await co_innner();

      CO_ASSERT_EQ(co_result, 1);
    }
  }());
}

auto async_update() -> xyco::runtime::Future<int> { co_return -1; }

TEST(NoRuntimeDeathTest, NeverRun) {
  auto evaluator = UpdateEvaluator();

  EXPECT_EXIT(
      {
        xyco::runtime::RuntimeCtx::set_ctx(nullptr);

        evaluator.update(async_update());

        std::quick_exit(evaluator.co_result);
      },
      testing::ExitedWithCode(0), "");
}

auto throw_uncaught_exception() -> xyco::runtime::Future<void> {
  throw std::runtime_error("Throw runtime error!");
  co_return;
}

TEST(RuntimeDeathTest, throw_in_block_on) {
  EXPECT_DEATH(
      {
        auto runtime = *xyco::runtime::Builder::new_multi_thread().build();

        runtime->block_on(throw_uncaught_exception());
      },
      "");
}

TEST(RuntimeDeathTest, throw_in_spawn) {
  EXPECT_DEATH(
      {
        auto runtime = *xyco::runtime::Builder::new_multi_thread()
                            .worker_threads(1)
                            .build();

        runtime->spawn(throw_uncaught_exception());

        std::this_thread::sleep_for(std::chrono::milliseconds(10000));
      },
      "");
}

class DropAsserter {
 public:
  DropAsserter(char member) : member_(member) { dropped_ = false; }

  DropAsserter(const DropAsserter &drop_asserter) = delete;

  DropAsserter(DropAsserter &&drop_asserter) noexcept {
    *this = std::move(drop_asserter);
  }

  auto operator=(const DropAsserter &drop_asserter) -> DropAsserter & = delete;

  auto operator=(DropAsserter &&drop_asserter) noexcept -> DropAsserter & {
    member_ = drop_asserter.member_;
    drop_asserter.member_ = -1;

    return *this;
  }

  static auto assert_drop() { std::quick_exit(dropped_ ? 0 : -1); }

  static auto drop(DropAsserter drop_asserter) -> xyco::runtime::Future<void> {
    co_return;
  }

  ~DropAsserter() {
    if (member_ != -1) {
      dropped_ = true;
    }
  }

 private:
  static std::atomic_bool dropped_;

  int member_{};
};

std::atomic_bool DropAsserter::dropped_ = false;

TEST(RuntimeDeathTest, drop_parameter) {
  auto drop_asserter = DropAsserter(2);

  EXPECT_EXIT(
      {
        auto runtime = *xyco::runtime::Builder::new_multi_thread().build();

        runtime->block_on(DropAsserter::drop(std::move(drop_asserter)));

        DropAsserter::assert_drop();
      },
      testing::ExitedWithCode(0), "");
}
