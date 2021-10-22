#ifndef XYCO_RUNTIME_RUNTIME_H_
#define XYCO_RUNTIME_RUNTIME_H_

#include <atomic>
#include <exception>
#include <thread>
#include <unordered_map>

#include "driver.h"
#include "future.h"
#include "net/driver/mod.h"

namespace xyco::runtime {
class Runtime;

class Worker {
  friend class Runtime;

 public:
  auto lanuch(Runtime *runtime) -> void;

  auto stop() -> void;

  auto get_native_id() const -> std::thread::id;

  auto get_epoll_registry() -> net::NetRegistry &;

 private:
  auto run_loop_once(Runtime *runtime) -> void;

  std::atomic_bool end_;
  std::thread ctx_;
  net::NetRegistry epoll_registry_;
  // (handle, future) -> co_await on a future object
  std::vector<std::pair<Handle<void>, FutureBase *>> handles_;
  std::mutex handle_mutex_;
};

class Runtime {
  friend class Worker;
  friend class Builder;
  friend class io::IoRegistry;

  class Privater {};

 public:
  template <typename T>
  auto spawn(Future<T> future) -> void {
    // resume once to skip initial_suspend
    auto handle = future.get_handle();
    spawn_catch_exception(std::move(future)).get_handle().resume();
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

  // Try to cancel the future and waited futures.
  // Futures may be cancelled when suspended or run to the end so the cancel
  // point is not fixed.
  // Only guarantee the coroutine will not be destroyed when executing.
  auto cancel_future(FutureBase *future) -> void;

  auto wake(Events &events) -> void;

  auto io_handle() -> GlobalRegistry *;

  auto time_handle() -> Registry *;

  auto blocking_handle() -> Registry *;

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
    } catch (std::exception e) {
      auto tid = std::this_thread::get_id();
      std::scoped_lock<std::mutex> lock_guard(worker_mutex_);
      auto pos = workers_.find(tid);
      if (workers_.size() == 1) {
        ERROR("unhandled coroutine exception, terminate the process");
        std::terminate();
      }
      ERROR("unhandled coroutine exception, kill current worker");
      pos->second->stop();
      workers_.erase(pos);
    }
  }

  auto deregister_future(Handle<void> future) -> Handle<void>;

  std::unordered_map<std::thread::id, std::unique_ptr<Worker>> workers_;
  std::mutex worker_mutex_;
  // (handle, nullptr) -> initial_suspend of a spawned async function
  // (handle, future) -> co_await on a future object
  std::vector<std::pair<Handle<void>, FutureBase *>> handles_;
  std::mutex handle_mutex_;
  std::unique_ptr<Driver> driver_;

  std::vector<FutureBase *> cancel_futures_;
  std::mutex cancel_futures_mutex_;

  auto (*on_start_f_)() -> void{};
  auto (*on_stop_f_)() -> void{};
};

class Builder {
 public:
  static auto new_multi_thread() -> Builder;

  auto worker_threads(uintptr_t val) -> Builder &;

  auto max_blocking_threads(uintptr_t val) -> Builder &;

  auto on_worker_start(auto (*f)()->void) -> Builder &;

  auto on_worker_stop(auto (*f)()->void) -> Builder &;

  [[nodiscard]] auto build() const -> io::IoResult<std::unique_ptr<Runtime>>;

 private:
  static auto default_f() -> void;

  uintptr_t worker_num_;
  uintptr_t blocking_num_;

  auto (*on_start_f_)() -> void;
  auto (*on_stop_f_)() -> void;
};
}  // namespace xyco::runtime

#endif  // XYCO_RUNTIME_RUNTIME_H_
