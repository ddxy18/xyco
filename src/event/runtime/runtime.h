#ifndef XYWEBSERVER_EVENT_RUNTIME_RUNTIME_H_
#define XYWEBSERVER_EVENT_RUNTIME_RUNTIME_H_

#include "driver.h"
#include "future.h"
#include "runtime_base.h"

namespace runtime {
class Runtime;
class Builder;

class Worker {
 public:
  explicit Worker(Runtime *runtime);

  auto run() -> void;

 private:
  Runtime *runtime_;
};

class Runtime : public RuntimeBase {
  friend class Worker;
  friend class Builder;

  class Privater {};

 public:
  template <typename T>
  auto spawn(Future<T> future) -> void {
    if (future.get_handle()) {
      mutex_.lock();
      handles_.insert(handles_.begin(), {future.get_handle(), nullptr});
      mutex_.unlock();
    }
  }

  template <typename T>
  auto spawn_blocking(std::function<T()> future) -> void {
    spawn(std::function<Future<T>()>([=]() -> Future<T> {
      auto res = co_await AsyncFuture<T>(
          std::function<T()>([=]() { return future(); }));
      co_return res;
    })());
  }

  auto register_future(FutureBase *future) -> void override {
    this->handles_.emplace(handles_.begin(), future->get_handle(), future);
  }

  auto io_handle() -> IoHandle * override { return driver_->net_handle(); }

  auto blocking_handle() -> IoHandle * override {
    return driver_->blocking_handle();
  }

  auto run() -> void;

  Runtime(Privater priv) {}

 private:
  std::vector<Worker> workers_;
  // (handle, nullptr) -> init_suspend of a spawned async function
  // (handle, future) -> co_await on a future object
  std::vector<std::pair<Handle<void>, FutureBase *>> handles_;
  std::mutex mutex_;
  std::unique_ptr<Driver> driver_;
};

class Builder {
 public:
  static auto new_multi_thread() -> Builder { return {}; }

  auto worker_threads(uintptr_t val) -> Builder & {
    worker_num_ = val;
    return *this;
  }

  auto max_blocking_threads(uintptr_t val) -> Builder & {
    blocking_num_ = val;
    return *this;
  }

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
