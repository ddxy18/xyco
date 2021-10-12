#ifndef XYWEBSERVER_TEST_UTILS_H_
#define XYWEBSERVER_TEST_UTILS_H_

#include <gtest/gtest.h>

#include <functional>
#include <gsl/pointers>

#include "runtime/future.h"
#include "runtime/runtime.h"

#define CO_ASSERT_EQ(val1, val2)               \
  ({                                           \
    auto f = [&]() { ASSERT_EQ(val1, val2); }; \
    f();                                       \
  })

class TestRuntimeCtxGuard {
 public:
  TestRuntimeCtxGuard(
      gsl::owner<std::function<runtime::Future<void>()> *> co_wrapper,
      bool in_runtime);

  ~TestRuntimeCtxGuard();

 private:
  gsl::owner<std::function<runtime::Future<void>()> *> co_wrapper_;
};

class TestRuntimeCtx {
 public:
  // run until co finished
  static auto co_run(std::function<runtime::Future<void>()> &&co) -> void;

  static auto co_run_no_wait(std::function<runtime::Future<void>()> &&co)
      -> TestRuntimeCtxGuard;

  static auto co_run_without_runtime(
      std::function<runtime::Future<void>()> &&co) -> TestRuntimeCtxGuard;
};

#endif  // XYWEBSERVER_TEST_UTILS_H_