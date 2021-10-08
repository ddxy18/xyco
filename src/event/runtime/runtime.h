#ifndef XYWEBSERVER_EVENT_RUNTIME_RUNTIME_H_
#define XYWEBSERVER_EVENT_RUNTIME_RUNTIME_H_

#include <atomic>
#include <exception>
#include <thread>

#include "driver.h"
#include "future.h"
#include "runtime_base.h"

namespace runtime {
class Runtime;
class Builder;
class Driver;

class Worker {
 public:
  explicit Worker(Runtime *runtime);

  auto run() -> void;

 private:
  Runtime *runtime_;
  std::atomic_bool end_;
};

class Runtime : public RuntimeBase {
  friend class Worker;
  friend class Builder;

  class Privater {};

 public:
  template <typename T>
  auto spawn(Future<T> future) -> void {
    auto future_wrapper = spawn_catch_exception(future);
    if (future_wrapper.get_handle()) {
      std::scoped_lock<std::mutex> lock_guard(mutex_);
      handles_.insert(handles_.begin(), {future_wrapper.get_handle(), nullptr});
    }
  }

  template <typename Fn>
  auto spawn_blocking(Fn &&f) -> void requires(std::is_invocable_v<Fn>) {
    using Return = decltype(f());

    spawn([=]() -> Future<Return> {
      co_return co_await AsyncFuture<Return>([=]() { return f(); });
    }());
  }

  auto register_future(FutureBase *future) -> void override;

  auto io_handle() -> IoHandle * override;

  auto blocking_handle() -> IoHandle * override;

  auto run() -> void;

  Runtime(Privater priv);

  Runtime(const Runtime &runtime) = delete;

  Runtime(Runtime &&runtime) = delete;

  auto operator=(const Runtime &runtime) -> Runtime & = delete;

  auto operator=(Runtime &&runtime) -> Runtime & = delete;

  ~Runtime();

 private:
  template <typename T>
  auto spawn_catch_exception(Future<T> future) -> Future<void> {
    try {
      co_await future;
    } catch (std::exception e) {
      std::this_thread::yield();
    }
  }

  std::vector<Worker> workers_;
  std::vector<std::thread> worker_ctx_;
  // (handle, nullptr) -> init_suspend of a spawned async function
  // (handle, future) -> co_await on a future object
  std::vector<std::pair<Handle<void>, FutureBase *>> handles_;
  std::mutex mutex_;
  std::unique_ptr<Driver> driver_;
  std::atomic_bool end_;
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
