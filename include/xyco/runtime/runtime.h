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
#include "xyco/utils/error.h"
#include "xyco/utils/logger.h"

namespace xyco::runtime {
class Runtime;

class Worker {
  friend class Runtime;
  friend class RuntimeBridge;
  friend class Builder;

 public:
  auto run_in_new_thread(Runtime *runtime) -> void;

  auto run_in_place(Runtime *runtime) -> void;

  auto stop() -> void;

  auto get_native_id() const -> std::thread::id;

 private:
  auto run_loop_once(Runtime *runtime) -> void;

  static auto init_in_thread(Runtime *runtime) -> void;

  std::atomic_bool end_;
  std::thread ctx_;
  // (handle, future) -> co_await on a future object
  std::vector<std::pair<Handle<void>, FutureBase *>> handles_;
  std::mutex handle_mutex_;
};

class Runtime {
  friend class Worker;
  friend class Builder;
  friend class RuntimeBridge;

  class Privater {};

 public:
  template <typename T>
  auto spawn(Future<T> future) -> void {
    spawn_impl(spawn_catch_exception(std::move(future)));
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
      runtime->in_place_worker_.stop();
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
      runtime->in_place_worker_.stop();
    };
    spawn_impl(blocking_future(this, std::move(future)));

    in_place_worker_.run_in_place(this);
  }

  Runtime(Privater priv,
          std::vector<std::function<void(Runtime *)>> &&registry_initializers);

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

  // Handle all unhandled exceptions of spawned coroutine.
  // more than one worker: kill the current worker
  // last worker: terminate the process
  template <typename T>
  auto spawn_catch_exception(Future<T> future) -> Future<void> {
    try {
      co_await future;
      // NOLINTNEXTLINE(bugprone-empty-catch)
    } catch (CancelException e) {
    } catch (std::exception e) {
      auto tid = std::this_thread::get_id();

      // try to lock strategy to avoid deadlock in situation:
      // ~Runtime owns the lock
      std::unique_lock<std::mutex> lock_guard(worker_mutex_,
                                              std::try_to_lock_t{});
      auto pos = workers_.find(tid);
      while (!pos->second->end_) {
        if (lock_guard || lock_guard.try_lock()) {
          if (workers_.size() == 1) {
            ERROR("unhandled coroutine exception, terminate the process");
            std::terminate();
          }
          ERROR("unhandled coroutine exception, kill current worker");
          pos->second->stop();
        }
      }
    }
  }

  std::unordered_map<std::thread::id, std::unique_ptr<Worker>> workers_;
  std::mutex worker_mutex_;
  Worker in_place_worker_;

  std::vector<std::thread::id> idle_workers_;
  std::mutex idle_workers_mutex_;

  // (handle, nullptr) -> initial_suspend of a spawned async function
  // (handle, future) -> co_await on a future object
  std::vector<std::pair<Handle<void>, FutureBase *>> handles_;
  std::mutex handle_mutex_;
  Driver driver_;

  uintptr_t worker_num_{};
  std::atomic_int init_worker_num_;
  std::mutex worker_launch_mutex_;
  std::condition_variable worker_launch_cv_;

  auto (*on_start_f_)() -> void{};
  auto (*on_stop_f_)() -> void{};
};

class Builder {
 public:
  static auto new_multi_thread() -> Builder;

  auto worker_threads(uintptr_t val) -> Builder &;

  template <typename Registry, typename... Args>
  auto registry(Args... args) -> Builder & {
    registry_initializers_.push_back([=](Runtime *runtime) {
      runtime->driver_.add_registry<Registry>(args...);
    });
    return *this;
  }

  auto on_worker_start(auto (*function)()->void) -> Builder &;

  auto on_worker_stop(auto (*function)()->void) -> Builder &;

  [[nodiscard]] auto build() -> utils::Result<std::unique_ptr<Runtime>>;

 private:
  static auto default_f() -> void;

  uintptr_t worker_num_{};

  auto (*on_start_f_)() -> void{};
  auto (*on_stop_f_)() -> void{};

  std::vector<std::function<void(Runtime *)>> registry_initializers_;
};
}  // namespace xyco::runtime

#endif  // XYCO_RUNTIME_RUNTIME_H_
