#ifndef XYWEBSERVER_TEST_UTILS_H_
#define XYWEBSERVER_TEST_UTILS_H_

#include <gtest/gtest.h>

#include <gsl/pointers>

#include "runtime/runtime.h"

#define CO_ASSERT_EQ(val1, val2)               \
  ({                                           \
    auto f = [&]() { ASSERT_EQ(val1, val2); }; \
    f();                                       \
  })

constexpr std::chrono::milliseconds time_deviation(7);

class TestRuntimeCtxGuard {
 public:
  TestRuntimeCtxGuard(
      gsl::owner<std::function<xyco::runtime::Future<void>()> *> co_wrapper,
      bool in_runtime);

  TestRuntimeCtxGuard(const TestRuntimeCtxGuard &guard) = delete;

  TestRuntimeCtxGuard(TestRuntimeCtxGuard &&guard) = delete;

  auto operator=(const TestRuntimeCtxGuard &guard)
      -> TestRuntimeCtxGuard & = delete;

  auto operator=(TestRuntimeCtxGuard &&guard) -> TestRuntimeCtxGuard & = delete;

  ~TestRuntimeCtxGuard();

 private:
  gsl::owner<std::function<xyco::runtime::Future<void>()> *> co_wrapper_;
};

class TestRuntimeCtx {
 public:
  // run until co finished
  template <typename Fn>
  static auto co_run(Fn &&co)
      -> void requires(std::is_invocable_r_v<xyco::runtime::Future<void>, Fn>) {
    // co_outer's lifetime is managed by the caller to avoid being destroyed
    // automatically before resumed.

    std::mutex mutex;
    std::unique_lock<std::mutex> lock_guard(mutex);
    std::condition_variable cv;

    auto co_outer = [&]() -> xyco::runtime::Future<void> {
      try {
        co_await co();
        cv.notify_one();
      } catch (std::exception e) {
        auto f = [&]() { ASSERT_NO_THROW(throw e); };
        f();
        cv.notify_one();
      }
    };
    runtime_->spawn(co_outer());
    cv.wait(lock_guard);
  }

  template <typename Fn>
  static auto co_run_no_wait(Fn &&co) -> TestRuntimeCtxGuard
      requires(std::is_invocable_r_v<xyco::runtime::Future<void>, Fn>) {
    auto *co_outer = gsl::owner<std::function<xyco::runtime::Future<void>()> *>(
        new std::function<xyco::runtime::Future<void>()>(
            [=]() -> xyco::runtime::Future<void> { co_await co(); }));

    return {co_outer, true};
  }

  template <typename Fn>
  static auto co_run_without_runtime(Fn &&co) -> TestRuntimeCtxGuard
      requires(std::is_invocable_r_v<xyco::runtime::Future<void>, Fn>) {
    auto *co_outer = gsl::owner<std::function<xyco::runtime::Future<void>()> *>(
        new std::function<xyco::runtime::Future<void>()>(
            [=]() -> xyco::runtime::Future<void> { co_await co(); }));

    return {co_outer, false};
  }

 private : static std::unique_ptr<xyco::runtime::Runtime> runtime_;
};

#endif  // XYWEBSERVER_TEST_UTILS_H_