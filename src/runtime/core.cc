module;

#include <functional>
#include <mutex>
#include <ranges>
#include <thread>
#include <vector>

module xyco.runtime_core;

import xyco.logging;

auto xyco::runtime::Worker::run_in_place(RuntimeCore *core) -> void {
  while (!suspend_flag_) {
    run_loop_once(core);
  }
  suspend_flag_ = false;
}

auto xyco::runtime::Worker::suspend() -> void { suspend_flag_ = true; }

auto xyco::runtime::Worker::id() const -> std::thread::id { return ctx_.get_id(); }

xyco::runtime::Worker::Worker(RuntimeCore *core, bool in_place) {
  if (in_place) {
    init_in_thread(core, true);
  } else {
    ctx_ = std::thread([this, core]() {
      init_in_thread(core);
      run_in_place(core);
    });
  }
}

xyco::runtime::Worker::~Worker() {
  if (ctx_.joinable()) {
    ctx_.join();
  }
}

auto xyco::runtime::Worker::init_in_thread(RuntimeCore *core, bool in_place) -> void {
  RuntimeCtxImpl::set_ctx(core);

  if (in_place) {
    core->driver_.add_thread();
  } else {
    // Workers have to add local registry to `Driver` and this modifies non
    // thread safe container in `Driver`. The container is read only after all
    // initializing completes. So wait until all workers complete initializing
    // to avoid concurrent write and read.
    std::unique_lock<std::mutex> worker_init_lock_guard(core->worker_launch_mutex_);
    core->driver_.add_thread();

    if (++core->init_worker_num_ == core->worker_num_) {
      worker_init_lock_guard.unlock();
      core->worker_launch_cv_.notify_all();
    } else {
      core->worker_launch_cv_.wait(worker_init_lock_guard);
    }
  }
}

auto xyco::runtime::Worker::run_loop_once(RuntimeCore *core) -> void {
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
  resume(core->handles_, core->handle_mutex_);
  if (suspend_flag_) {
    return;
  }

  // drive both local and global registry
  core->driver_.poll();
}

auto xyco::runtime::RuntimeCore::register_future(FutureBase *future) -> void {
  std::scoped_lock<std::mutex> lock_guard(handle_mutex_);
  handles_.emplace(handles_.begin(), future->get_handle(), future);
}

auto xyco::runtime::RuntimeCore::driver() -> Driver & { return driver_; }

auto xyco::runtime::RuntimeCore::wake(Events &events) -> void {
  for (auto &event_ptr : events) {
    logging::trace("wake {}", *event_ptr);
    auto *future = event_ptr->future_;
    // Unbinds `future_` first to avoid contaminating the event's next coroutine
    event_ptr->future_ = nullptr;
    std::scoped_lock<std::mutex> lock_guard(handle_mutex_);
    handles_.emplace(handles_.begin(), future->get_handle(), future);
  }
  events.clear();
}

auto xyco::runtime::RuntimeCore::wake_local(Events &events) -> void {
  auto &worker = workers_.find(std::this_thread::get_id())->second;
  for (auto &event_ptr : events) {
    logging::trace("wake local {}", *event_ptr);
    auto *future = event_ptr->future_;
    event_ptr->future_ = nullptr;
    std::scoped_lock<std::mutex> lock_guard(worker->handle_mutex_);
    worker->handles_.emplace(worker->handles_.begin(), future->get_handle(), future);
  }
  events.clear();
}

xyco::runtime::RuntimeCore::RuntimeCore(
    std::vector<std::function<void(Driver *)>> &&registry_initializers, uint16_t worker_num)
    : driver_(std::move(registry_initializers)),
      worker_num_(worker_num),
      in_place_worker_(this, true) {
  for ([[maybe_unused]] auto idx : std::views::iota(0, static_cast<int>(worker_num_))) {
    auto worker = std::make_unique<Worker>(this);
    workers_.emplace(worker->id(), std::move(worker));
  }
}

xyco::runtime::RuntimeCore::~RuntimeCore() {
  for (const auto &[_, worker] : workers_) {
    worker->suspend();
  }
}

thread_local xyco::runtime::RuntimeCore *xyco::runtime::RuntimeCtxImpl::core_;

auto xyco::runtime::RuntimeCtxImpl::is_in_ctx() -> bool { return core_ != nullptr; }

auto xyco::runtime::RuntimeCtxImpl::set_ctx(RuntimeCore *core) -> void { core_ = core; }

auto xyco::runtime::RuntimeCtxImpl::get_ctx() -> RuntimeCore * { return core_; }
