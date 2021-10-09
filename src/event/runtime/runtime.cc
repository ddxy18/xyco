#include "runtime.h"

#include <thread>

thread_local runtime::RuntimeBase *runtime::RuntimeCtx::runtime_ = nullptr;

auto runtime::Worker::run(Runtime *runtime) -> void {
  RuntimeCtx::set_ctx(runtime);
  while (!end_) {
    std::unique_lock<std::mutex> lock_guard(runtime->handle_mutex_);
    while (!runtime->handles_.empty()) {
      auto [handle, future] = runtime->handles_.back();
      runtime->handles_.pop_back();
      lock_guard.unlock();
      if (future == nullptr || future->poll_wrapper()) {
        handle.resume();
      }
      lock_guard.lock();
    }
    lock_guard.unlock();
    runtime->driver_->poll();
  }
}

auto runtime::Runtime::register_future(FutureBase *future) -> void {
  std::scoped_lock<std::mutex> lock_guard(handle_mutex_);
  this->handles_.emplace(handles_.begin(), future->get_handle(), future);
}

auto runtime::Runtime::io_handle() -> IoHandle * {
  return driver_->net_handle();
}

auto runtime::Runtime::blocking_handle() -> IoHandle * {
  return driver_->blocking_handle();
}

auto runtime::Runtime::run() -> void {
  for (auto &worker : workers_) {
    worker = std::make_unique<Worker>();
    worker_ctx_.emplace_back(&Worker::run, worker.get(), this);
  }
}

runtime::Runtime::Runtime(Privater priv) {}

runtime::Runtime::~Runtime() {
  for (auto &worker : workers_) {
    worker->end_ = true;
  }
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
  auto runtime = std::make_unique<Runtime>(Runtime::Privater());
  runtime->workers_ = std::vector<std::unique_ptr<Worker>>(worker_num_);
  runtime->driver_ = std::make_unique<Driver>(blocking_num_);

  return IoResult<std::unique_ptr<Runtime>>::ok(std::move(runtime));
}