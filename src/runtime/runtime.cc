#include "xyco/runtime/runtime.h"

#include <ranges>

#include "xyco/runtime/runtime_ctx.h"

auto xyco::runtime::Worker::run_in_place(Runtime *runtime) -> void {
  while (!suspend_flag_) {
    run_loop_once(runtime);
  }
  suspend_flag_ = false;
}

auto xyco::runtime::Worker::suspend() -> void { suspend_flag_ = true; }

auto xyco::runtime::Worker::id() const -> std::thread::id {
  return ctx_.get_id();
}

xyco::runtime::Worker::Worker(Runtime *runtime, bool in_place) {
  if (in_place) {
    init_in_thread(runtime, true);
  } else {
    ctx_ = std::thread([this, runtime]() {
      init_in_thread(runtime);
      run_in_place(runtime);
    });
  }
}

xyco::runtime::Worker::~Worker() {
  if (ctx_.joinable()) {
    ctx_.join();
  }
}

auto xyco::runtime::Worker::init_in_thread(Runtime *runtime, bool in_place)
    -> void {
  RuntimeCtx::set_ctx(runtime);

  if (in_place) {
    runtime->driver_.add_thread();
  } else {
    // Workers have to add local registry to `Driver` and this modifies non
    // thread safe container in `Driver`. The container is read only after all
    // initializing completes. So wait until all workers complete initializing
    // to avoid concurrent write and read.
    std::unique_lock<std::mutex> worker_init_lock_guard(
        runtime->worker_launch_mutex_);
    runtime->driver_.add_thread();

    if (++runtime->init_worker_num_ == runtime->worker_num_) {
      worker_init_lock_guard.unlock();
      runtime->worker_launch_cv_.notify_all();
    } else {
      runtime->worker_launch_cv_.wait(worker_init_lock_guard);
    }
  }
}

auto xyco::runtime::Worker::run_loop_once(Runtime *runtime) -> void {
  auto resume = [&end = suspend_flag_](auto &handles, auto &handle_mutex) {
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
  if (suspend_flag_) {
    return;
  }
  // resume global future
  resume(runtime->handles_, runtime->handle_mutex_);
  if (suspend_flag_) {
    return;
  }

  // drive both local and global registry
  runtime->driver_.poll();
}

xyco::runtime::Runtime::Runtime(
    std::vector<std::function<void(Driver *)>> &&registry_initializers,
    uint16_t worker_num)
    : driver_(std::move(registry_initializers)),
      worker_num_(worker_num),
      in_place_worker_(this, true) {
  for ([[maybe_unused]] auto idx :
       std::views::iota(0, static_cast<int>(worker_num_))) {
    auto worker = std::make_unique<Worker>(this);
    workers_.emplace(worker->id(), std::move(worker));
  }
}

xyco::runtime::Runtime::~Runtime() {
  for (const auto &[_, worker] : workers_) {
    worker->suspend();
  }
}

auto xyco::runtime::Builder::new_multi_thread() -> Builder { return {}; }

auto xyco::runtime::Builder::worker_threads(uint16_t val) -> Builder & {
  worker_num_ = val;
  return *this;
}

auto xyco::runtime::Builder::build()
    -> std::expected<std::unique_ptr<Runtime>, std::nullptr_t> {
  return std::make_unique<Runtime>(std::move(registry_initializers_),
                                   worker_num_);
}
