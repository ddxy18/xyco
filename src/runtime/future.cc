#include "future.h"

auto runtime::Future<void>::PromiseType::get_return_object() -> Future<void> {
  return Future(Handle<promise_type>::from_promise(*this));
}

// NOLINTNEXTLINE(readability-convert-member-functions-to-static)
auto runtime::Future<void>::PromiseType::initial_suspend() noexcept
    -> InitFuture {
  return {};
}

auto runtime::Future<void>::PromiseType::final_suspend() noexcept
    -> FinalAwaitable {
  return {waiting_};
}

auto runtime::Future<void>::PromiseType::unhandled_exception() -> void {
  exception_ptr_ = std::current_exception();
}

auto runtime::Future<void>::PromiseType::return_void() -> void {}

runtime::Future<void>::Awaitable::Awaitable(Future<void> &future) noexcept
    : future_(future) {}

[[nodiscard]] auto runtime::Future<void>::Awaitable::await_ready()
    const noexcept -> bool {
  if (future_.self_) {
    return future_.self_.done();
  }
  return false;
}

auto runtime::Future<void>::Awaitable::await_suspend(
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

auto runtime::Future<void>::Awaitable::await_resume() -> void {
  auto ptr = future_.self_.promise().exception_ptr_;
  if (ptr) {
    std::rethrow_exception(ptr);
  }
}

runtime::Future<void>::Future(Handle<promise_type> self) : self_(self) {}

auto runtime::Future<void>::operator co_await() -> Awaitable {
  return Awaitable(*this);
}

[[nodiscard]] auto runtime::Future<void>::poll(Handle<void> self)
    -> Poll<void> {
  return Pending();
}

[[nodiscard]] auto runtime::Future<void>::poll_wrapper() -> bool {
  return !std::holds_alternative<Pending>(poll(waiting_));
}