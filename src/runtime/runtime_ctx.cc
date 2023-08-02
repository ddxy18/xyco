#include "xyco/runtime/runtime_ctx.h"

#include "xyco/runtime/runtime.h"

thread_local xyco::runtime::RuntimeBridge xyco::runtime::RuntimeCtx::runtime_ =
    RuntimeBridge(nullptr);

auto xyco::runtime::RuntimeBridge::register_future(FutureBase *future) -> void {
  std::scoped_lock<std::mutex> lock_guard(runtime_->handle_mutex_);
  runtime_->handles_.emplace(runtime_->handles_.begin(), future->get_handle(),
                             future);
}

auto xyco::runtime::RuntimeBridge::driver() -> Driver & {
  return runtime_->driver_;
}

auto xyco::runtime::RuntimeBridge::get_runtime() -> Runtime * {
  return runtime_;
}

auto xyco::runtime::RuntimeBridge::wake(Events &events) -> void {
  for (auto &event_ptr : events) {
    TRACE("wake {}", *event_ptr);
    auto *future = event_ptr->future_;
    // Unbinds `future_` first to avoid contaminating the event's next coroutine
    event_ptr->future_ = nullptr;
    std::scoped_lock<std::mutex> lock_guard(runtime_->handle_mutex_);
    runtime_->handles_.emplace(runtime_->handles_.begin(), future->get_handle(),
                               future);
  }
  events.clear();
}

auto xyco::runtime::RuntimeBridge::wake_local(Events &events) -> void {
  auto &worker = runtime_->workers_.find(std::this_thread::get_id())->second;
  for (auto &event_ptr : events) {
    TRACE("wake local {}", *event_ptr);
    auto *future = event_ptr->future_;
    event_ptr->future_ = nullptr;
    std::scoped_lock<std::mutex> lock_guard(worker->handle_mutex_);
    worker->handles_.emplace(worker->handles_.begin(), future->get_handle(),
                             future);
  }
  events.clear();
}

xyco::runtime::RuntimeBridge::RuntimeBridge(Runtime *runtime)
    : runtime_(runtime) {}

auto xyco::runtime::RuntimeCtx::is_in_ctx() -> bool {
  return runtime_.runtime_ != nullptr;
}

auto xyco::runtime::RuntimeCtx::set_ctx(Runtime *runtime) -> void {
  runtime_ = RuntimeBridge(runtime);
}

auto xyco::runtime::RuntimeCtx::get_ctx() -> RuntimeBridge * {
  return &runtime_;
}
