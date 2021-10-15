#include "runtime.h"

thread_local xyco::runtime::Runtime *xyco::runtime::RuntimeCtx::runtime_ =
    nullptr;

auto xyco::runtime::Worker::lanuch(Runtime *runtime) -> void {
  ctx_ = std::thread([=]() {
    RuntimeCtx::set_ctx(runtime);
    while (!end_) {
      run_loop_once(runtime);
    }
  });
}

auto xyco::runtime::Worker::stop() -> void {
  end_ = true;
  if (std::this_thread::get_id() == get_native_id()) {
    ctx_.detach();
  } else {
    ctx_.join();
  }
}

auto xyco::runtime::Worker::get_native_id() const -> std::thread::id {
  return ctx_.get_id();
}

auto xyco::runtime::Worker::get_epoll_registry() -> net::NetRegistry & {
  return epoll_registry_;
}

auto xyco::runtime::Worker::run_loop_once(Runtime *runtime) -> void {
  {
    std::unique_lock<std::mutex> lock_guard(handle_mutex_);
    while (!handles_.empty()) {
      auto [handle, future] = handles_.back();
      handles_.pop_back();
      lock_guard.unlock();
      if (future->poll_wrapper()) {
        handle.resume();
      }
      lock_guard.lock();
    }
  }
  {
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
  }
  Events events;
  epoll_registry_.select(events, net::NetRegistry::MAX_TIMEOUT_MS).unwrap();
  for (Event &ev : events) {
    if (ev.future_ != nullptr) {
      TRACE("process {}", ev);
      std::scoped_lock<std::mutex> lock_guard(handle_mutex_);
      handles_.emplace(handles_.begin(), ev.future_->get_handle(), ev.future_);
    }
  }
  runtime->driver_->poll();
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

auto xyco::runtime::Runtime::register_future(FutureBase *future) -> void {
  std::scoped_lock<std::mutex> lock_guard(handle_mutex_);
  this->handles_.emplace(handles_.begin(), future->get_handle(), future);
}

auto xyco::runtime::Runtime::io_handle() -> GlobalRegistry * {
  return driver_->net_handle();
}

auto xyco::runtime::Runtime::blocking_handle() -> Registry * {
  return driver_->blocking_handle();
}

xyco::runtime::Runtime::Runtime(Privater priv) {}

xyco::runtime::Runtime::~Runtime() {
  for (const auto &[tid, worker] : workers_) {
    worker->stop();
  }
}

auto xyco::runtime::Builder::new_multi_thread() -> Builder { return {}; }

auto xyco::runtime::Builder::worker_threads(uintptr_t val) -> Builder & {
  worker_num_ = val;
  return *this;
}

auto xyco::runtime::Builder::max_blocking_threads(uintptr_t val) -> Builder & {
  blocking_num_ = val;
  return *this;
}

auto xyco::runtime::Builder::build() const
    -> io::IoResult<std::unique_ptr<Runtime>> {
  auto runtime = std::make_unique<Runtime>(Runtime::Privater());
  runtime->driver_ = std::make_unique<Driver>(blocking_num_);
  for (auto i = 0; i < worker_num_; i++) {
    auto worker = std::make_unique<Worker>();
    worker->lanuch(runtime.get());
    runtime->workers_.emplace(worker->get_native_id(), std::move(worker));
  }

  return io::IoResult<std::unique_ptr<Runtime>>::ok(std::move(runtime));
}