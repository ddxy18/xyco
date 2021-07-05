#ifndef XYWEBSERVER_EVENT_RUNTIME_FUTURE_H_
#define XYWEBSERVER_EVENT_RUNTIME_FUTURE_H_

#include <experimental/coroutine>
#include <thread>
#include <variant>
#include <vector>

namespace runtime {
class RuntimeBase;

template <typename T>
using Handle = std::experimental::coroutine_handle<T>;

// 'runtime!=nullptr' if the thread runs in the context of a runtime.It ensures
// that only coroutines spawned to a runtime will be actually executed.
extern thread_local RuntimeBase *runtime;

class Pending {};

template <typename T>
class Ready {
 public:
  T inner_;
};

template <>
class Ready<void> {};

template <typename T>
using Poll = std::variant<Ready<T>, Pending>;

class FutureBase {
 public:
  // Returns
  // true means the coroutine ends.
  [[nodiscard]] virtual auto poll_wrapper() -> bool = 0;

  // Returns
  // async function--self
  // Future object--the co_awaiter
  virtual auto get_handle() -> Handle<void> = 0;
};

class InitFuture : public std::experimental::suspend_always {
 public:
  static auto await_ready() -> bool { return runtime != nullptr; }
};

// Future<Output> can be used in 2 different mode:
// - Use it as the return type of async functions
// Just declare it as the function's return type. The caller coroutine will be
// resumed automatically after it completes.
// - co_await on this object directly
// In this mode, no promise will be created. Users should implement poll() to
// control when the caller coroutine will be resumed.
// It works as follow:
// Call this->poll(). If it returns Poll::Ready, the coroutine will execute as
// usually. Otherwise, the coroutine suspends and it's the user's responsibility
// to wake up the coroutine sometime later. Usually we register the handle to a
// reactor.
template <typename Output>
class Future : public FutureBase {
 public:
  class PromiseType;
  class Awaitable;
  using promise_type = PromiseType;

  class PromiseType {
    friend class Future::Awaitable;

   public:
    auto get_return_object() -> Future<Output> {
      return Future(Handle<promise_type>::from_promise(*this));
    }

    auto initial_suspend() noexcept -> InitFuture { return InitFuture(); }

    auto final_suspend() noexcept -> std::experimental::suspend_never {
      if (waiting_) {
        waiting_.resume();
      }
      return std::experimental::suspend_never();
    }

    auto unhandled_exception() -> void {}

    auto return_value(Output &value) -> Output {
      return_ = value;
      return value;
    }

    auto return_value(Output &&value) -> Output {
      return_ = value;
      return value;
    }

   private:
    Output return_;
    Handle<void> waiting_;
  };

  class Awaitable {
   public:
    explicit Awaitable(Future<Output> &future) noexcept : future_(future) {}

    [[nodiscard]] auto await_ready() const noexcept -> bool {
      if (future_.self_) {
        return future_.self_.done();
      }
      return false;
    }

    auto await_suspend(Handle<void> waiting_coroutine) noexcept -> bool {
      future_.waiting_ = waiting_coroutine;
      // async function's return type
      if (future_.self_) {
        future_.self_.promise().waiting_ = waiting_coroutine;
        return true;
      }
      // Future object
      auto res = future_.poll(waiting_coroutine);
      if (std::holds_alternative<Pending>(res)) {
        return true;
      }
      future_.return_ = std::get<Ready<Output>>(res).inner_;
      return false;
    }

    auto await_resume() -> Output {
      if (future_.self_) {
        return future_.self_.promise().return_;
      }
      return future_.return_;
    }

   private:
    Future<Output> &future_;
  };

  // co_await Future object should pass nullptr
  explicit Future(Handle<promise_type> self) : self_(self) {}

  auto operator co_await() -> Awaitable { return Awaitable{*this}; }

  [[nodiscard]] virtual auto poll(Handle<void> self) -> Poll<Output> {
    return Pending();
  }

  [[nodiscard]] auto poll_wrapper() -> bool override {
    auto res = poll(waiting_);
    if (std::holds_alternative<Pending>(res)) {
      return false;
    }
    return_ = std::get<Ready<Output>>(res).inner_;
    return true;
  }

  inline auto get_handle() -> Handle<void> override {
    return self_ == nullptr ? waiting_ : self_;
  }

 private:
  Output return_;
  Handle<promise_type> self_;
  Handle<void> waiting_;
};

template <>
class Future<void> : public FutureBase {
 public:
  class PromiseType;
  class Awaitable;
  using promise_type = PromiseType;

  class PromiseType {
    friend class Future::Awaitable;

   public:
    auto get_return_object() -> Future<void> {
      return Future(Handle<promise_type>::from_promise(*this));
    }

    static auto initial_suspend() noexcept -> InitFuture {
      return InitFuture();
    }

    auto final_suspend() noexcept -> std::experimental::suspend_never {
      if (waiting_) {
        waiting_.resume();
      }
      return std::experimental::suspend_never();
    }

    auto unhandled_exception() -> void {}

    auto return_void() -> void {}

   private:
    Handle<void> waiting_;
  };

  class Awaitable {
   public:
    explicit Awaitable(Future<void> &future) noexcept : future_(future) {}

    [[nodiscard]] auto await_ready() const noexcept -> bool {
      if (future_.self_) {
        return future_.self_.done();
      }
      return false;
    }

    auto await_suspend(Handle<void> waiting_coroutine) noexcept -> bool {
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

    auto await_resume() -> void {}

   private:
    Future<void> &future_;
  };

  explicit Future(Handle<promise_type> self) : self_(self) {}

  auto operator co_await() -> Awaitable { return Awaitable{*this}; }

  [[nodiscard]] virtual auto poll(Handle<void> self) -> Poll<void> {
    return Pending();
  }

  [[nodiscard]] auto poll_wrapper() -> bool override {
    return !std::holds_alternative<Pending>(poll(waiting_));
  }

  inline auto get_handle() -> Handle<void> override {
    return self_ == nullptr ? waiting_ : self_;
  }

 private:
  Handle<promise_type> self_;
  Handle<void> waiting_;
};
}  // namespace runtime

#endif  // XYWEBSERVER_EVENT_RUNTIME_FUTURE_H_