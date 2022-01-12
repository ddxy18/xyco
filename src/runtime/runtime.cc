#include "runtime.h"

#include <gsl/pointers>

thread_local xyco::runtime::Runtime *xyco::runtime::RuntimeCtx::runtime_ =
    nullptr;

auto xyco::runtime::Worker::lanuch(Runtime *runtime) -> void {
  ctx_ = std::thread([=]() {
    runtime->on_start_f_();
    RuntimeCtx::set_ctx(runtime);
    while (!end_) {
      run_loop_once(runtime);
    }
    runtime->on_stop_f_();

    std::unique_lock<std::mutex> lock_guard(runtime->idle_workers_mutex_);
    runtime->idle_workers_.push_back(std::this_thread::get_id());
  });
}

auto xyco::runtime::Worker::stop() -> void { end_ = true; }

auto xyco::runtime::Worker::get_native_id() const -> std::thread::id {
  return ctx_.get_id();
}

auto xyco::runtime::Worker::get_epoll_registry() -> net::NetRegistry & {
  return epoll_registry_;
}

auto xyco::runtime::Worker::run_loop_once(Runtime *runtime) -> void {
  // cancel future
  {
    std::scoped_lock<std::mutex> lock_guard(
        runtime->cancel_future_handles_mutex_);
    decltype(runtime->cancel_future_handles_) cancel_fail_futures;
    for (auto &handle : runtime->cancel_future_handles_) {
      if (runtime->deregister_future(handle.promise().pending_future()) !=
              nullptr ||
          (handle != nullptr && handle.done())) {
        handle.destroy();
      } else {
        cancel_fail_futures.push_back(handle);
      }
    }
    runtime->cancel_future_handles_ = cancel_fail_futures;
  }

  auto resume = [&end = this->end_](auto &handles, auto &handle_mutex) {
    std::unique_lock<std::mutex> lock_guard(handle_mutex);
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
  Events events;
  epoll_registry_.select(events, net::NetRegistry::MAX_TIMEOUT).unwrap();
  for (Event &ev : events) {
    if (ev.future_ != nullptr) {
      TRACE("process {}", ev);
      std::scoped_lock<std::mutex> lock_guard(handle_mutex_);
      handles_.emplace(handles_.begin(), ev.future_->get_handle(), ev.future_);
    }
  }
  runtime->driver_->poll();

  // try to clear idle workers
  {
    // try to lock strategy to avoid deadlock in situation:
    // ~Runtime owns the lock before current thread
    std::unique_lock<std::mutex> worker_lock_guard(runtime->worker_mutex_,
                                                   std::try_to_lock_t{});
    if (worker_lock_guard) {
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
}

auto xyco::runtime::Runtime::register_future(FutureBase *future) -> void {
  std::scoped_lock<std::mutex> lock_guard(handle_mutex_);
  this->handles_.emplace(handles_.begin(), future->get_handle(), future);
}

auto xyco::runtime::Runtime::cancel_future(Handle<PromiseBase> handle) -> void {
  std::scoped_lock<std::mutex> lock_guard(cancel_future_handles_mutex_);
  cancel_future_handles_.push_back(handle);
}

auto xyco::runtime::Runtime::wake(Events &events) -> void {
  for (Event &ev : events) {
    if (ev.future_ != nullptr) {
      TRACE("process {}", ev);
      register_future(ev.future_);
    }
  }
  events.clear();
}

auto xyco::runtime::Runtime::io_handle() -> GlobalRegistry * {
  return driver_->io_handle();
}

auto xyco::runtime::Runtime::time_handle() -> Registry * {
  return driver_->time_handle();
}

auto xyco::runtime::Runtime::blocking_handle() -> Registry * {
  return driver_->blocking_handle();
}

xyco::runtime::Runtime::Runtime(Privater priv) {}

xyco::runtime::Runtime::~Runtime() {
  std::unique_lock<std::mutex> lock_guard_(worker_mutex_);
  for (const auto &[tid, worker] : workers_) {
    worker->stop();
    worker->ctx_.join();
  }
}

auto xyco::runtime::Runtime::deregister_future(Handle<void> future)
    -> Handle<void> {
  {
    std::scoped_lock<std::mutex> lock_guard(handle_mutex_);
    auto pos = std::remove_if(std::begin(handles_), std::end(handles_),
                              [=](auto pair) { return pair.first == future; });
    if (pos != std::end(handles_)) {
      handles_.erase(pos, handles_.end());
      return future;
    }
  }

  for (auto &[id, worker] : workers_) {
    std::scoped_lock<std::mutex> lock_guard(worker->handle_mutex_);
    auto handles = worker->handles_;
    auto pos = std::remove_if(std::begin(handles), std::end(handles),
                              [=](auto pair) { return pair.first == future; });
    if (pos != std::end(handles)) {
      handles.erase(pos, handles_.end());
      return future;
    }
  }

  return nullptr;
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

auto xyco::runtime::Builder::max_blocking_threads(uintptr_t val) -> Builder & {
  blocking_num_ = val;
  return *this;
}

auto xyco::runtime::Builder::on_worker_start(auto (*f)()->void) -> Builder & {
  on_start_f_ = f;
  return *this;
}

auto xyco::runtime::Builder::on_worker_stop(auto (*f)()->void) -> Builder & {
  on_stop_f_ = f;
  return *this;
}

auto xyco::runtime::Builder::build() const
    -> io::IoResult<std::unique_ptr<Runtime>> {
  auto runtime = std::make_unique<Runtime>(Runtime::Privater());
  runtime->on_start_f_ = on_start_f_;
  runtime->on_stop_f_ = on_stop_f_;
  runtime->driver_ = std::make_unique<Driver>(blocking_num_);
  for (auto i = 0; i < worker_num_; i++) {
    auto worker = std::make_unique<Worker>();
    worker->lanuch(runtime.get());
    runtime->workers_.emplace(worker->get_native_id(), std::move(worker));
  }

  return io::IoResult<std::unique_ptr<Runtime>>::ok(std::move(runtime));
}

auto xyco::runtime::Builder::default_f() -> void {}