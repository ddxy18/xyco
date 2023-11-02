#include "xyco/runtime/runtime.h"

#include <gsl/pointers>

#include "xyco/runtime/runtime_ctx.h"

auto xyco::runtime::Worker::run_in_new_thread(Runtime *runtime) -> void {
  ctx_ = std::thread([this, runtime]() {
    init_in_thread(runtime);

    run_in_place(runtime);

    std::unique_lock<std::mutex> lock_guard(runtime->idle_workers_mutex_);
    runtime->idle_workers_.push_back(std::this_thread::get_id());
  });
}

auto xyco::runtime::Worker::run_in_place(Runtime *runtime) -> void {
  end_ = false;
  while (!end_) {
    run_loop_once(runtime);
  }
}

auto xyco::runtime::Worker::init_in_thread(Runtime *runtime) -> void {
  RuntimeCtx::set_ctx(runtime);

  // Workers have to add local registry to `Driver` and this modifies non
  // thread safe container in `Driver`. The container is read only after all
  // initializing completes. So wait until all workers complete initializing
  // to avoid concurrent write and read.
  std::unique_lock<std::mutex> worker_init_lock_guard(
      runtime->worker_launch_mutex_);
  runtime->driver_.add_thread();
  // Count in-place worker
  auto worker_count = static_cast<int>(runtime->worker_num_ + 1);
  if (++runtime->init_worker_num_ == worker_count) {
    worker_init_lock_guard.unlock();
    runtime->worker_launch_cv_.notify_all();
  } else {
    runtime->worker_launch_cv_.wait(worker_init_lock_guard);
  }
}

auto xyco::runtime::Worker::stop() -> void { end_ = true; }

auto xyco::runtime::Worker::get_native_id() const -> std::thread::id {
  return ctx_.get_id();
}

auto xyco::runtime::Worker::run_loop_once(Runtime *runtime) -> void {
  auto resume = [&end = end_](auto &handles, auto &handle_mutex) {
    std::unique_lock<std::mutex> lock_guard(handle_mutex, std::try_to_lock);
    if (!lock_guard) {
      return;
    }

    while (!end && !handles.empty()) {
      auto [handle, future] = handles.back();
      handles.pop_back();
      lock_guard.unlock();
      if (future == nullptr || future->poll_wrapper()) {
        handle.resume();
      }
      lock_guard.lock();
    }
  };

  // resume local future
  resume(handles_, handle_mutex_);
  if (end_) {
    return;
  }
  // resume global future
  resume(runtime->handles_, runtime->handle_mutex_);
  if (end_) {
    return;
  }

  // drive both local and global registry
  runtime->driver_.poll();

  // try to clear idle workers
  {
    // try to lock strategy to avoid deadlock in situation:
    // ~Runtime owns the lock before current thread
    std::unique_lock<std::mutex> worker_lock_guard(runtime->worker_mutex_,
                                                   std::try_to_lock);
    if (!worker_lock_guard) {
      return;
    }

    std::scoped_lock<std::mutex> idle_worker_lock_guard(
        runtime->idle_workers_mutex_);
    for (auto &tid : runtime->idle_workers_) {
      auto pos = runtime->workers_.find(tid);
      if (pos != std::end(runtime->workers_)) {
        pos->second->ctx_.join();
        runtime->workers_.erase(pos);
      }
    }
    runtime->idle_workers_.clear();
  }
}

xyco::runtime::Runtime::Runtime(
    std::vector<std::function<void(Driver *)>> &&registry_initializers,
    int worker_num)
    : driver_(std::move(registry_initializers)), worker_num_(worker_num) {
  for (auto i = 0; i < worker_num; i++) {
    auto worker = std::make_unique<Worker>();
    worker->run_in_new_thread(this);
    workers_.emplace(worker->get_native_id(), std::move(worker));
  }
  // initialized last to avoid blocking other workers
  in_place_worker_.init_in_thread(this);
}

xyco::runtime::Runtime::~Runtime() {
  std::unique_lock<std::mutex> lock_guard_(worker_mutex_);
  for (const auto &[tid, worker] : workers_) {
    worker->stop();
    worker->ctx_.join();
  }
}

auto xyco::runtime::Builder::new_multi_thread() -> Builder { return {}; }

auto xyco::runtime::Builder::worker_threads(uintptr_t val) -> Builder & {
  worker_num_ = val;
  return *this;
}

auto xyco::runtime::Builder::build()
    -> std::expected<std::unique_ptr<Runtime>, std::nullptr_t> {
  return std::make_unique<Runtime>(std::move(registry_initializers_),
                                   worker_num_);
}
