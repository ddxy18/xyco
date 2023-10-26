module;

#include <coroutine>
#include <iostream>

#include "boost/stacktrace.hpp"

module xyco.future;

auto xyco::runtime::Future<void>::PromiseType::get_return_object()
    -> Future<void> {
  return Future(Handle<promise_type>::from_promise(*this));
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
  std::cerr << boost::stacktrace::stacktrace();
}

auto xyco::runtime::Future<void>::PromiseType::return_void() -> void {
  if (future_ != nullptr) {
    future_->return_ = true;
  }
}

auto xyco::runtime::Future<void>::PromiseType::future() -> FutureBase * {
  return future_;
}

auto xyco::runtime::Future<void>::PromiseType::set_waited(
    Handle<PromiseBase> future) -> void {
  if (future_ != nullptr) {
    future_->waited_ = future;
  }
}

auto xyco::runtime::Future<void>::operator co_await() -> Awaitable<void> {
  return Awaitable(this);
}

auto xyco::runtime::Future<void>::poll([[maybe_unused]] Handle<void> self)
    -> Poll<void> {
  return Pending();
}

auto xyco::runtime::Future<void>::poll_wrapper() -> bool {
  return !std::holds_alternative<Pending>(poll(waiting_));
}

auto xyco::runtime::Future<void>::cancel() -> void {
  cancelled_ = true;
  if (waited_ != nullptr) {
    waited_.promise().future()->cancel();
  }
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
  if (self_) {
    self_.promise().future_ = this;
  }

  return *this;
}

xyco::runtime::Future<void>::~Future() {
  if (self_) {
    // Outermost coroutine has to execute after its future dropped, so we ignore
    // it here.
    if (!waiting_) {
      self_.promise().future_ = nullptr;
    }
    if (waiting_) {
      self_.destroy();
    }
  }
}