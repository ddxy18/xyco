#include "future.h"

auto xyco::runtime::Future<void>::PromiseType::get_return_object()
    -> Future<void> {
  return Future(Handle<promise_type>::from_promise(*this));
}

// NOLINTNEXTLINE(readability-convert-member-functions-to-static)
auto xyco::runtime::Future<void>::PromiseType::initial_suspend() noexcept
    -> InitFuture {
  return {};
}

auto xyco::runtime::Future<void>::PromiseType::final_suspend() noexcept
    -> FinalAwaitable {
  // 'future_ == nullptr' means:
  // future dropped but coroutine frame still exists(now only happen in
  // Runtime::spawn)
  return {future_ != nullptr ? future_->waiting_ : nullptr};
}

auto xyco::runtime::Future<void>::PromiseType::unhandled_exception() -> void {
  future_->exception_ptr_ = std::current_exception();
}

auto xyco::runtime::Future<void>::PromiseType::return_void() -> void {}

auto xyco::runtime::Future<void>::PromiseType::set_waited(FutureBase *future)
    -> void {
  if (future_ != nullptr) {
    future_->waited_ = future;
  }
}

auto xyco::runtime::Future<void>::operator co_await() -> Awaitable<void> {
  return Awaitable(*this);
}

auto xyco::runtime::Future<void>::poll(Handle<void> self) -> Poll<void> {
  return Pending();
}

auto xyco::runtime::Future<void>::poll_wrapper() -> bool {
  return !std::holds_alternative<Pending>(poll(waiting_));
}

auto xyco::runtime::Future<void>::pending_future() -> FutureBase * {
  return waited_ != nullptr ? waited_->pending_future() : this;
}

xyco::runtime::Future<void>::Future(Handle<promise_type> self) : self_(self) {
  if (self_) {
    self_.promise().future_ = this;
  }
}

xyco::runtime::Future<void>::Future(Future<void> &&future) noexcept {
  *this = std::move(future);
}

auto xyco::runtime::Future<void>::operator=(Future<void> &&future) noexcept
    -> Future<void> & {
  self_ = future.self_;
  future.self_ = nullptr;
  waiting_ = future.waiting_;
  future.waiting_ = nullptr;
  waited_ = future.waited_;
  future.waited_ = nullptr;
  self_.promise().future_ = this;

  return *this;
}

xyco::runtime::Future<void>::~Future() {
  if (self_) {
    // Outer coroutine has to execute after its future dropped, so we ignore it
    // here.
    self_.promise().future_ = nullptr;
    if (waiting_) {
      self_.destroy();
    }
  }
}