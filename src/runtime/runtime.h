#ifndef XYCO_RUNTIME_RUNTIME_H_
#define XYCO_RUNTIME_RUNTIME_H_

#include <atomic>
#include <exception>
#include <thread>
#include <unordered_map>

#include "driver.h"
#include "future.h"
#include "net/driver/mod.h"

namespace runtime {
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
    spawn_catch_exception(future).get_handle().resume();
    if (future.get_handle()) {
      std::scoped_lock<std::mutex> lock_guard(handle_mutex_);
      handles_.insert(handles_.begin(), {future.get_handle(), nullptr});
    }
  }

  template <typename Fn>
  auto spawn_blocking(Fn &&f) -> void requires(std::is_invocable_v<Fn>) {
    using Return = decltype(f());

    spawn([=]() -> Future<Return> {
      co_return co_await AsyncFuture<Return>([=]() { return f(); });
    }());
  }

  auto wake(runtime::Events &events) -> void;

  auto io_handle() -> runtime::GlobalRegistry *;

  auto blocking_handle() -> runtime::Registry *;

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

  auto register_future(FutureBase *future) -> void;

  std::unordered_map<std::thread::id, std::unique_ptr<Worker>> workers_;
  std::mutex worker_mutex_;
  // (handle, nullptr) -> initial_suspend of a spawned async function
  // (handle, future) -> co_await on a future object
  std::vector<std::pair<Handle<void>, FutureBase *>> handles_;
  std::mutex handle_mutex_;
  std::unique_ptr<Driver> driver_;
};

class Builder {
 public:
  static auto new_multi_thread() -> Builder;

  auto worker_threads(uintptr_t val) -> Builder &;

  auto max_blocking_threads(uintptr_t val) -> Builder &;

  auto enable_io() -> Builder &;

  auto enable_time() -> Builder &;

  auto on_thread_start(auto (*f)()->void) -> Builder &;

  auto on_thread_stop(auto (*f)()->void) -> Builder &;

  [[nodiscard]] auto build() const -> io::IoResult<std::unique_ptr<Runtime>>;

 private:
  uintptr_t worker_num_;
  uintptr_t blocking_num_;
  bool io_;
  bool time_;
};
}  // namespace runtime

#endif  // XYCO_RUNTIME_RUNTIME_H_
