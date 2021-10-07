#ifndef XYWEBSERVER_EVENT_RUNTIME_RUNTIME_H_
#define XYWEBSERVER_EVENT_RUNTIME_RUNTIME_H_

#include <atomic>
#include <exception>
#include <thread>

#include "driver.h"
#include "future.h"

namespace runtime {
class Runtime;

using IoHandle = reactor::Poll;

class Worker {
  friend class Runtime;

 public:
  auto run(Runtime *runtime) -> void;

 private:
  std::atomic_bool end_;
};

class Runtime {
  friend class Worker;
  friend class Builder;

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

  auto register_future(FutureBase *future) -> void;

  auto io_handle() -> IoHandle *;

  auto blocking_handle() -> IoHandle *;

  auto run() -> void;

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
      auto pos = std::find_if(
          std::begin(worker_ctx_), std::end(worker_ctx_),
          [&](auto &worker_thread) { return worker_thread.get_id() == tid; });
      if (workers_.size() == 1) {
        ERROR("unhandled coroutine exception, terminate the process");
        std::terminate();
      }
      ERROR("unhandled coroutine exception, kill current worker");
      workers_[std::distance(worker_ctx_.begin(), pos)]->end_ = true;
      pos->detach();
      worker_ctx_.erase(pos);
      workers_.erase(workers_.begin() +
                     std::distance(worker_ctx_.begin(), pos));
    }
  }

  std::vector<std::unique_ptr<Worker>> workers_;
  std::vector<std::thread> worker_ctx_;
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

  [[nodiscard]] auto build() const -> IoResult<std::unique_ptr<Runtime>>;

 private:
  uintptr_t worker_num_;
  uintptr_t blocking_num_;
  bool io_;
  bool time_;
};
}  // namespace runtime

#endif  // XYWEBSERVER_EVENT_RUNTIME_RUNTIME_H_
