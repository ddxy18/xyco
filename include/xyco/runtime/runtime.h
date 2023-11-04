#ifndef XYCO_RUNTIME_RUNTIME_H_
#define XYCO_RUNTIME_RUNTIME_H_

#include <atomic>
#include <condition_variable>
#include <exception>
#include <functional>
#include <mutex>
#include <thread>
#include <unordered_map>
#include <vector>

#include "xyco/runtime/driver.h"
#include "xyco/runtime/future.h"
#include "xyco/utils/logger.h"

import xyco.error;

namespace xyco::runtime {
class Runtime;

class Worker {
  friend class RuntimeBridge;

 public:
  auto run_in_place(Runtime *runtime) -> void;

  auto suspend() -> void;

  auto id() const -> std::thread::id;

  Worker(Runtime *runtime, bool in_place = false);

  ~Worker();

  Worker(const Worker &) = delete;

  Worker(Worker &&) = delete;

  auto operator=(const Worker &) -> Worker & = delete;

  auto operator=(Worker &&) -> Worker & = delete;

 private:
  auto run_loop_once(Runtime *runtime) -> void;

  static auto init_in_thread(Runtime *runtime, bool in_place = false) -> void;

  std::thread ctx_;
  std::atomic_bool suspend_flag_;

  // (handle, future) -> co_await on a future object
  std::vector<std::pair<Handle<void>, FutureBase *>> handles_;
  std::mutex handle_mutex_;
};

class Runtime {
  friend class Worker;
  friend class RuntimeBridge;

 public:
  template <typename T>
  auto spawn(Future<T> future) -> void {
    spawn_impl(spawn_with_exception_handling(std::move(future)));
  }

  // Blocks the current thread until `future` completes.
  // Note: Current thread is strictly restricted to the thread creating the
  // runtime, and also the thread must not be a worker of another runtime.
  template <typename T>
  auto block_on(Future<T> future) -> T {
    std::optional<T> result;
    // NOLINTNEXTLINE(cppcoreguidelines-avoid-capturing-lambda-coroutines)
    auto blocking_future = [&](Runtime *runtime, auto future) -> Future<void> {
      result = co_await future;
      runtime->in_place_worker_.suspend();
    };
    spawn_impl(blocking_future(this, std::move(future)));

    in_place_worker_.run_in_place(this);

    // NOLINTNEXTLINE(bugprone-unchecked-optional-access)
    return std::move(*result);
  }

  template <>
  auto block_on(Future<void> future) -> void {
    auto blocking_future = [](Runtime *runtime, auto future) -> Future<void> {
      co_await future;
      runtime->in_place_worker_.suspend();
    };
    spawn_impl(blocking_future(this, std::move(future)));

    in_place_worker_.run_in_place(this);
  }

  Runtime(std::vector<std::function<void(Driver *)>> &&registry_initializers,
          uint16_t worker_num);

  Runtime(const Runtime &runtime) = delete;

  Runtime(Runtime &&runtime) = delete;

  auto operator=(const Runtime &runtime) -> Runtime & = delete;

  auto operator=(Runtime &&runtime) -> Runtime & = delete;

  ~Runtime();

 private:
  template <typename T>
  auto spawn_impl(Future<T> future) -> void {
    auto handle = future.get_handle();
    if (handle) {
      std::scoped_lock<std::mutex> lock_guard(handle_mutex_);
      handles_.insert(handles_.begin(), {handle, nullptr});
    }
  }

  template <typename T>
  auto spawn_with_exception_handling(Future<T> future) -> Future<void> {
    try {
      co_await future;
      // NOLINTNEXTLINE(bugprone-empty-catch)
    } catch (CancelException e) {
    } catch (const std::exception &e) {
      ERROR("Uncaught coroutine exception: {}", e.what());
      throw;
    }
  }

  // (handle, nullptr) -> initial_suspend of a spawned async function
  // (handle, future) -> co_await on a future object
  std::vector<std::pair<Handle<void>, FutureBase *>> handles_;
  std::mutex handle_mutex_;
  Driver driver_;

  uint16_t worker_num_{};
  std::atomic_int init_worker_num_;
  std::mutex worker_launch_mutex_;
  std::condition_variable worker_launch_cv_;

  Worker in_place_worker_;
  // Put last to guarantee all workers destructed before other data members.
  std::unordered_map<std::thread::id, std::unique_ptr<Worker>> workers_;
};

class Builder {
 public:
  static auto new_multi_thread() -> Builder;

  auto worker_threads(uint16_t val) -> Builder &;

  template <typename Registry, typename... Args>
  auto registry(Args... args) -> Builder & {
    registry_initializers_.push_back(
        [=](Driver *driver) { driver->add_registry<Registry>(args...); });
    return *this;
  }

  [[nodiscard]] auto build()
      -> std::expected<std::unique_ptr<Runtime>, std::nullptr_t>;

 private:
  uint16_t worker_num_{};

  std::vector<std::function<void(Driver *)>> registry_initializers_;
};
}  // namespace xyco::runtime

#endif  // XYCO_RUNTIME_RUNTIME_H_
