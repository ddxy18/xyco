#ifndef XYWEBSERVER_EVENT_RUNTIME_FUTURE_H_
#define XYWEBSERVER_EVENT_RUNTIME_FUTURE_H_

#include <experimental/coroutine>
#include <variant>

namespace runtime {
class RuntimeBase;

template <typename T>
using Handle = std::experimental::coroutine_handle<T>;

class RuntimeCtx {
 public:
  static auto is_in_ctx() -> bool { return runtime_ != nullptr; }

  static auto set_ctx(RuntimeBase *runtime) -> void { runtime_ = runtime; }

  static auto get_ctx() -> RuntimeBase * { return runtime_; }

 private:
  thread_local static RuntimeBase *runtime_;
};

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

class InitFuture {
 public:
  // NOLINTNEXTLINE(readability-convert-member-functions-to-static)
  auto await_ready() -> bool { return RuntimeCtx::is_in_ctx(); }

  auto await_suspend(Handle<void> h) const -> void {}

  auto await_resume() const -> void {}
};

class FinalAwaitable {
 public:
  FinalAwaitable(Handle<void> waiting_coroutine)
      : waiting_(waiting_coroutine), ready_(false) {}

  [[nodiscard]] auto await_ready() const noexcept -> bool { return ready_; }

  auto await_suspend(Handle<void> waiting_coroutine) noexcept {
    return waiting_ ? waiting_ : std::experimental::noop_coroutine();
  }

  auto await_resume() noexcept -> void {}

 private:
  Handle<void> waiting_;
  const bool ready_;
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

    auto initial_suspend() noexcept -> InitFuture { return {}; }

    auto final_suspend() noexcept -> FinalAwaitable { return {waiting_}; }

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
  explicit Future(Handle<promise_type> self) : self_(self), return_() {}

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
    auto get_return_object() -> Future<void>;

    // NOLINTNEXTLINE(readability-convert-member-functions-to-static)
    auto initial_suspend() noexcept -> InitFuture;

    auto final_suspend() noexcept -> FinalAwaitable;

    auto unhandled_exception() -> void;

    auto return_void() -> void;

   private:
    Handle<void> waiting_;
  };

  class Awaitable {
   public:
    explicit Awaitable(Future<void> &future) noexcept;

    [[nodiscard]] auto await_ready() const noexcept -> bool;

    auto await_suspend(Handle<void> waiting_coroutine) noexcept -> bool;

    auto await_resume() -> void;

   private:
    Future<void> &future_;
  };

  explicit Future(Handle<promise_type> self);

  auto operator co_await() -> Awaitable;

  [[nodiscard]] virtual auto poll(Handle<void> self) -> Poll<void>;

  [[nodiscard]] auto poll_wrapper() -> bool override;

  inline auto get_handle() -> Handle<void> override {
    return self_ == nullptr ? waiting_ : self_;
  }

 private:
  Handle<promise_type> self_;
  Handle<void> waiting_;
};
}  // namespace runtime

#endif  // XYWEBSERVER_EVENT_RUNTIME_FUTURE_H_