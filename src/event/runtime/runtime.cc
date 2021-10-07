#include "runtime.h"

#include <thread>

thread_local runtime::RuntimeBase *runtime::RuntimeCtx::runtime_ = nullptr;

runtime::Worker::Worker(Runtime *runtime) : runtime_(runtime) {}

auto runtime::Worker::run() -> void {
  RuntimeCtx::set_ctx(runtime_);
  while (!runtime_->end_) {
    std::unique_lock<std::mutex> lock_guard(runtime_->mutex_);
    while (!runtime_->handles_.empty()) {
      auto [handle, future] = runtime_->handles_.back();
      runtime_->handles_.pop_back();
      lock_guard.unlock();
      if (future == nullptr || future->poll_wrapper()) {
        handle.resume();
      }
      lock_guard.lock();
    }
    lock_guard.unlock();
    runtime_->driver_->poll();
  }
}

auto runtime::Runtime::register_future(FutureBase *future) -> void {
  std::scoped_lock<std::mutex> lock_guard(mutex_);
  this->handles_.emplace(handles_.begin(), future->get_handle(), future);
}

auto runtime::Runtime::io_handle() -> IoHandle * {
  return driver_->net_handle();
}

auto runtime::Runtime::blocking_handle() -> IoHandle * {
  return driver_->blocking_handle();
}

auto runtime::Runtime::run() -> void {
  for (auto worker : workers_) {
    worker_ctx_.emplace_back(&Worker::run, worker);
  }
}

runtime::Runtime::Runtime(Privater priv) : driver_(nullptr) {}

runtime::Runtime::~Runtime() {
  end_ = true;
  for (auto &t : worker_ctx_) {
    t.join();
  }
}

auto runtime::Builder::new_multi_thread() -> Builder { return {}; }

auto runtime::Builder::worker_threads(uintptr_t val) -> Builder & {
  worker_num_ = val;
  return *this;
}

auto runtime::Builder::max_blocking_threads(uintptr_t val) -> Builder & {
  blocking_num_ = val;
  return *this;
}

auto runtime::Builder::build() const -> IoResult<std::unique_ptr<Runtime>> {
  auto r = std::make_unique<Runtime>(Runtime::Privater());
  r->workers_ = std::vector<Worker>(worker_num_, Worker(r.get()));
  r->driver_ = std::make_unique<Driver>(blocking_num_);

  return IoResult<std::unique_ptr<Runtime>>::ok(std::move(r));
}