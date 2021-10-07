#include "utils.h"

#include <condition_variable>
#include <mutex>

#include "utils/result.h"

// co_outer's lifetime is managed by the caller to avoid being destroyed
// automatically before resumed.
auto TestRuntimeCtx::co_run(std::function<runtime::Future<void>()> &&co)
    -> void {
  auto run = []() {
    auto runtime = runtime::Builder::new_multi_thread()
                       .worker_threads(1)
                       .max_blocking_threads(1)
                       .build()
                       .unwrap();
    runtime->run();

    return runtime;
  };

  static auto runtime = run();

  std::mutex mutex;
  std::unique_lock<std::mutex> lock_guard(mutex);
  std::condition_variable cv;

  auto co_outer = [&]() -> runtime::Future<void> {
    co_await co();
    cv.notify_one();
  };
  runtime->spawn(co_outer());
  cv.wait(lock_guard);
}

auto TestRuntimeCtx::co_run_no_wait(std::function<runtime::Future<void>()> &&co)
    -> TestRuntimeCtxGuard {
  auto *co_outer = gsl::owner<std::function<runtime::Future<void>()> *>(
      new std::function<runtime::Future<void>()>(
          [=]() -> runtime::Future<void> { co_await co(); }));

  return {co_outer, true};
}

auto TestRuntimeCtx::co_run_without_runtime(
    std::function<runtime::Future<void>()> &&co) -> TestRuntimeCtxGuard {
  auto *co_outer = gsl::owner<std::function<runtime::Future<void>()> *>(
      new std::function<runtime::Future<void>()>(
          [=]() -> runtime::Future<void> { co_await co(); }));

  return {co_outer, false};
}

TestRuntimeCtxGuard::TestRuntimeCtxGuard(
    gsl::owner<std::function<runtime::Future<void>()> *> co_wrapper,
    bool in_runtime)
    : co_wrapper_(co_wrapper) {
  auto rt = in_runtime ? runtime::Builder::new_multi_thread().build().unwrap()
                       : nullptr;
  runtime::RuntimeCtx::set_ctx(rt.get());
  (*co_wrapper_)();
}

TestRuntimeCtxGuard::~TestRuntimeCtxGuard() {
  runtime::RuntimeCtx::set_ctx(nullptr);
  delete co_wrapper_;
}