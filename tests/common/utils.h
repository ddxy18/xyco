#ifndef XYCO_TESTS_COMMON_UTILS_H_
#define XYCO_TESTS_COMMON_UTILS_H_

#include <gtest/gtest.h>

#include <gsl/pointers>

#include "xyco/fs/file.h"
#include "xyco/io/extra.h"
#include "xyco/net/listener.h"
#include "xyco/runtime/runtime.h"
#include "xyco/time/clock.h"

template <typename T1, typename T2>
constexpr auto CO_ASSERT_EQ(T1 val1, T2 val2) {
  auto assert_eq_wrapper = [&]() { ASSERT_EQ(val1, val2); };
  assert_eq_wrapper();
}

constexpr std::chrono::milliseconds wait_interval(5);

class TestRuntimeCtx {
 public:
  // run until co finished
  template <typename T>
  static auto co_run(xyco::runtime::Future<T> coroutine,
                     std::vector<std::chrono::milliseconds> time_steps = {})
      -> void {
    // co_outer's lifetime is managed by the caller to avoid being destroyed
    // automatically before resumed.

    std::mutex mutex;
    std::unique_lock<std::mutex> lock_guard(mutex);
    std::condition_variable condition_variable;

    // NOLINTNEXTLINE(cppcoreguidelines-avoid-capturing-lambda-coroutines)
    auto co_outer = [&]() -> xyco::runtime::Future<void> {
      std::thread clock_driver([&]() {
        for (const auto &step : time_steps) {
          std::this_thread::sleep_for(clock_interval_);
          xyco::time::FrozenClock::advance(step);
        }
      });
      co_await coroutine;
      clock_driver.join();
      condition_variable.notify_one();
    };
    runtime_->spawn(co_outer());
    condition_variable.wait(lock_guard);
  }

  static auto runtime() -> xyco::runtime::Runtime * { return runtime_.get(); }

  static auto init() -> void;

 private:
  // IO syscall in every iteration costs 2ms at most. Some coroutines like
  // `timeout` have to experience several iterations to reach the pending point.
  // So assigns a relatively large interval here.
  static constexpr std::chrono::milliseconds clock_interval_{10};

  static std::unique_ptr<xyco::runtime::Runtime> runtime_;
};

#endif  // XYCO_TESTS_COMMON_UTILS_H_
