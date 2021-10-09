#include "runtime.h"

thread_local runtime::Runtime *runtime::RuntimeCtx::runtime_ = nullptr;

auto runtime::Worker::lanuch(Runtime *runtime) -> void {
  ctx_ = std::thread([=]() {
    RuntimeCtx::set_ctx(runtime);
    while (!end_) {
      run_loop_once(runtime);
    }
  });
}

auto runtime::Worker::stop() -> void {
  end_ = true;
  if (std::this_thread::get_id() == get_native_id()) {
    ctx_.detach();
  } else {
    ctx_.join();
  }
}

auto runtime::Worker::get_native_id() const -> std::thread::id {
  return ctx_.get_id();
}

auto runtime::Worker::run_loop_once(Runtime *runtime) -> void {
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

runtime::Runtime::Runtime(Privater priv) {}

runtime::Runtime::~Runtime() {
  for (const auto &[tid, worker] : workers_) {
    worker->stop();
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
  runtime->driver_ = std::make_unique<Driver>(blocking_num_);
  for (auto i = 0; i < worker_num_; i++) {
    auto worker = std::make_unique<Worker>();
    worker->lanuch(runtime.get());
    runtime->workers_.emplace(worker->get_native_id(), std::move(worker));
  }

  return IoResult<std::unique_ptr<Runtime>>::ok(std::move(runtime));
}