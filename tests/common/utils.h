#ifndef XYCO_TESTS_COMMON_UTILS_GLOBAL_H_
#define XYCO_TESTS_COMMON_UTILS_GLOBAL_H_

#include <gtest/gtest.h>

#include <gsl/pointers>

#include "fs/file.h"
#include "io/extra.h"
#include "net/listener.h"
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
  static auto co_run(Fn &&coroutine)
      -> void requires(std::is_invocable_r_v<xyco::runtime::Future<void>, Fn>) {
    // co_outer's lifetime is managed by the caller to avoid being destroyed
    // automatically before resumed.

    std::mutex mutex;
    std::unique_lock<std::mutex> lock_guard(mutex);
    std::condition_variable condition_variable;

    auto co_outer = [&]() -> xyco::runtime::Future<void> {
      try {
        co_await coroutine();
        condition_variable.notify_one();
      } catch (std::exception e) {
        [&]() { ASSERT_NO_THROW(throw e); }();
        condition_variable.notify_one();
      }
    };
    runtime_->spawn(co_outer());
    condition_variable.wait(lock_guard);
  }

  template <typename Fn>
  static auto co_run_no_wait(Fn &&coroutine) -> TestRuntimeCtxGuard
      requires(std::is_invocable_r_v<xyco::runtime::Future<void>, Fn>) {
    auto *co_outer = gsl::owner<std::function<xyco::runtime::Future<void>()> *>(
        new std::function<xyco::runtime::Future<void>()>(
            [=]() -> xyco::runtime::Future<void> { co_await coroutine(); }));

    return {co_outer, true};
  }

  template <typename Fn>
  static auto co_run_without_runtime(Fn &&coroutine) -> TestRuntimeCtxGuard
      requires(std::is_invocable_r_v<xyco::runtime::Future<void>, Fn>) {
    auto *co_outer = gsl::owner<std::function<xyco::runtime::Future<void>()> *>(
        new std::function<xyco::runtime::Future<void>()>(
            [=]() -> xyco::runtime::Future<void> { co_await coroutine(); }));

    return {co_outer, false};
  }

 private:
  static std::unique_ptr<xyco::runtime::Runtime> runtime_;
};

#endif  // XYCO_TESTS_COMMON_UTILS_GLOBAL_H_