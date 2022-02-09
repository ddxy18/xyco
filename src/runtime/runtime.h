#ifndef XYCO_RUNTIME_RUNTIME_H_
#define XYCO_RUNTIME_RUNTIME_H_

#include <atomic>
#include <exception>
#include <mutex>
#include <thread>
#include <unordered_map>
#include <vector>

#include "driver.h"
#include "future.h"

namespace xyco::runtime {
class Runtime;

class RuntimeCtx {
 public:
  static auto is_in_ctx() -> bool { return runtime_ != nullptr; }

  static auto set_ctx(Runtime *runtime) -> void { runtime_ = runtime; }

  static auto get_ctx() -> Runtime * { return runtime_; }

 private:
  thread_local static Runtime *runtime_;
};

class Worker {
  friend class Runtime;

 public:
  auto lanuch(Runtime *runtime) -> void;

  auto stop() -> void;

  auto get_native_id() const -> std::thread::id;

 private:
  auto run_loop_once(Runtime *runtime) -> void;

  std::atomic_bool end_;
  std::thread ctx_;
  // (handle, future) -> co_await on a future object
  std::vector<std::pair<Handle<void>, FutureBase *>> handles_;
  std::mutex handle_mutex_;
};

class Runtime {
  friend class Worker;
  friend class Builder;
  friend class Driver;

  class Privater {};

 public:
  template <typename T>
  auto spawn(Future<T> future) -> void {
    auto handle = spawn_catch_exception(std::move(future)).get_handle();
    if (handle) {
      std::scoped_lock<std::mutex> lock_guard(handle_mutex_);
      handles_.insert(handles_.begin(), {handle, nullptr});
    }
  }

  template <typename Fn>
  auto spawn_blocking(Fn &&f) -> void requires(std::is_invocable_v<Fn>) {
    using Return = decltype(f());

    spawn([=]() -> Future<Return> {
      co_return co_await AsyncFuture<Return>([=]() { return f(); });
    }());
  }

  auto register_future(FutureBase *future) -> void;

  auto driver() -> Driver &;

  Runtime(Privater priv);

  Runtime(const Runtime &runtime) = delete;

  Runtime(Runtime &&runtime) = delete;

  auto operator=(const Runtime &runtime) -> Runtime & = delete;

  auto operator=(Runtime &&runtime) -> Runtime & = delete;

  ~Runtime();

 private:
  // Handle all unhandled exceptions of spawned coroutine.
  // more than one worker: kill the current worker
  // last worker: terminate the process
  template <typename T>
  auto spawn_catch_exception(Future<T> future) -> Future<void> {
    try {
      co_await future;
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

  auto wake(Events &events) -> void;

  auto wake_local(Events &events) -> void;

  std::unordered_map<std::thread::id, std::unique_ptr<Worker>> workers_;
  std::mutex worker_mutex_;

  std::vector<std::thread::id> idle_workers_;
  std::mutex idle_workers_mutex_;

  // (handle, nullptr) -> initial_suspend of a spawned async function
  // (handle, future) -> co_await on a future object
  std::vector<std::pair<Handle<void>, FutureBase *>> handles_;
  std::mutex handle_mutex_;
  Driver driver_;

  auto (*on_start_f_)() -> void{};
  auto (*on_stop_f_)() -> void{};
};

class Builder {
 public:
  static auto new_multi_thread() -> Builder;

  auto worker_threads(uintptr_t val) -> Builder &;

  auto max_blocking_threads(uintptr_t val) -> Builder &;

  template <typename Registry, typename... Args>
  auto registry(Args... args) -> Builder & {
    registries_.push_back([=](Runtime *runtime) {
      runtime->driver_.add_registry<Registry>(args...);
    });
    return *this;
  }

  auto on_worker_start(auto (*f)()->void) -> Builder &;

  auto on_worker_stop(auto (*f)()->void) -> Builder &;

  [[nodiscard]] auto build() const -> io::IoResult<std::unique_ptr<Runtime>>;

 private:
  static auto default_f() -> void;

  uintptr_t worker_num_;

  auto (*on_start_f_)() -> void;
  auto (*on_stop_f_)() -> void;

  std::vector<std::function<void(Runtime *)>> registries_;
};
}  // namespace xyco::runtime

#endif  // XYCO_RUNTIME_RUNTIME_H_
