#include "runtime.h"

#include <thread>

thread_local runtime::RuntimeBase *runtime::RuntimeCtx::runtime_ = nullptr;

runtime::Worker::Worker(Runtime *runtime) : runtime_(runtime) {}

auto runtime::Worker::run() -> void {
  // runtime::runtime = runtime_;
  RuntimeCtx::set_ctx(runtime_);
  while (true) {
    while (!runtime_->handles_.empty()) {
      runtime_->mutex_.lock();
      auto [handle, future] = runtime_->handles_.back();
      runtime_->handles_.pop_back();
      runtime_->mutex_.unlock();
      if (future == nullptr || future->poll_wrapper()) {
        handle.resume();
      }
    }
    runtime_->driver_->poll();
  }
}

auto runtime::Runtime::register_future(FutureBase *future) -> void {
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
    auto t = std::thread(&Worker::run, worker);
    t.detach();
  }
}

runtime::Runtime::Runtime(Privater priv) {}

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

  return Ok<std::unique_ptr<Runtime>, IoError>(std::move(r));
}