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
  return {waiting_};
}

auto xyco::runtime::Future<void>::PromiseType::unhandled_exception() -> void {
  exception_ptr_ = std::current_exception();
}

auto xyco::runtime::Future<void>::PromiseType::return_void() -> void {}

xyco::runtime::Future<void>::Awaitable::Awaitable(Future<void> &future) noexcept
    : future_(future) {}

[[nodiscard]] auto xyco::runtime::Future<void>::Awaitable::await_ready()
    const noexcept -> bool {
  if (future_.self_) {
    return future_.self_.done();
  }
  return false;
}

auto xyco::runtime::Future<void>::Awaitable::await_suspend(
    Handle<void> waiting_coroutine) noexcept -> bool {
  future_.waiting_ = waiting_coroutine;
  // async function's return type
  if (future_.self_) {
    future_.self_.promise().waiting_ = waiting_coroutine;
    return true;
  }
  // Future object
  auto res = future_.poll(waiting_coroutine);
  return std::holds_alternative<Pending>(res);
}

auto xyco::runtime::Future<void>::Awaitable::await_resume() -> void {
  auto ptr = future_.self_ ? future_.self_.promise().exception_ptr_ : nullptr;
  if (ptr) {
    std::rethrow_exception(ptr);
  }
}

auto xyco::runtime::Future<void>::operator co_await() -> Awaitable {
  return Awaitable(*this);
}

[[nodiscard]] auto xyco::runtime::Future<void>::poll(Handle<void> self)
    -> Poll<void> {
  return Pending();
}

[[nodiscard]] auto xyco::runtime::Future<void>::poll_wrapper() -> bool {
  return !std::holds_alternative<Pending>(poll(waiting_));
}

xyco::runtime::Future<void>::Future(Handle<promise_type> self) : self_(self) {}

xyco::runtime::Future<void>::Future(Future<void> &&future) noexcept {
  *this = std::move(future);
}

auto xyco::runtime::Future<void>::operator=(Future<void> &&future) noexcept
    -> Future<void> & {
  self_ = future.self_;
  future.self_ = nullptr;
  waiting_ = future.waiting_;
  future.waiting_ = nullptr;

  return *this;
}

xyco::runtime::Future<void>::~Future() {
  if (self_ && waiting_) {
    self_.destroy();
  }
}