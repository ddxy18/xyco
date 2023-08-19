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
  runtime->on_stop_f_();
}

auto xyco::runtime::Worker::init_in_thread(Runtime *runtime) -> void {
  runtime->on_start_f_();
  RuntimeCtx::set_ctx(runtime);

  // Workers have to add local registry to `Driver` and this modifies non
  // thread safe container in `Driver`. The container is read only after all
  // initializing completes. So wait until all workers complete initializing
  // to avoid concurrent write and read.
  std::unique_lock<std::mutex> worker_init_lock_guard(
      runtime->worker_launch_mutex_);
  runtime->driver_.add_thread();
  // Count in-place worker
  auto worker_count = runtime->worker_num_ + 1;
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
  auto resume = [runtime, &end = end_](auto &handles, auto &handle_mutex) {
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
    Privater priv,
    std::vector<std::function<void(Runtime *)>> &&registry_initializers)
    : driver_(std::move(registry_initializers)) {}

xyco::runtime::Runtime::~Runtime() {
  std::unique_lock<std::mutex> lock_guard_(worker_mutex_);
  for (const auto &[tid, worker] : workers_) {
    worker->stop();
    worker->ctx_.join();
  }
}

auto xyco::runtime::Builder::new_multi_thread() -> Builder {
  Builder builder{};
  builder.on_start_f_ = default_f;
  builder.on_stop_f_ = default_f;
  return builder;
}

auto xyco::runtime::Builder::worker_threads(uintptr_t val) -> Builder & {
  worker_num_ = val;
  return *this;
}

auto xyco::runtime::Builder::on_worker_start(auto (*function)()->void)
    -> Builder & {
  on_start_f_ = function;
  return *this;
}

auto xyco::runtime::Builder::on_worker_stop(auto (*function)()->void)
    -> Builder & {
  on_stop_f_ = function;
  return *this;
}

auto xyco::runtime::Builder::build()
    -> utils::Result<std::unique_ptr<Runtime>> {
  auto runtime = std::make_unique<Runtime>(Runtime::Privater(),
                                           std::move(registry_initializers_));
  runtime->on_start_f_ = on_start_f_;
  runtime->on_stop_f_ = on_stop_f_;

  runtime->worker_num_ = worker_num_;
  for (auto i = 0; i < worker_num_; i++) {
    auto worker = std::make_unique<Worker>();
    worker->run_in_new_thread(runtime.get());
    runtime->workers_.emplace(worker->get_native_id(), std::move(worker));
  }
  // initialized last to avoid blocking other workers
  runtime->in_place_worker_.init_in_thread(runtime.get());

  return runtime;
}

auto xyco::runtime::Builder::default_f() -> void {}
