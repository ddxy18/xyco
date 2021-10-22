#ifndef XYCO_RUNTIME_FUTURE_H_
#define XYCO_RUNTIME_FUTURE_H_

#include <exception>
#include <experimental/coroutine>
#include <optional>
#include <variant>
#include <vector>

namespace xyco::runtime {
class Runtime;
template <typename Output>
class Future;

template <typename T>
using Handle = std::experimental::coroutine_handle<T>;

class RuntimeCtx {
 public:
  static auto is_in_ctx() -> bool { return runtime_ != nullptr; }

  static auto set_ctx(Runtime *runtime) -> void { runtime_ = runtime; }

  static auto get_ctx() -> Runtime * { return runtime_; }

 private:
  thread_local static Runtime *runtime_;
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

  virtual auto pending_future() -> FutureBase * = 0;

  FutureBase() = default;

  FutureBase(const FutureBase &future_base) = delete;

  FutureBase(FutureBase &&future_base) = delete;

  auto operator=(const FutureBase &future_base) -> const FutureBase & = delete;

  auto operator=(FutureBase &&future_base) -> const FutureBase & = delete;

  virtual ~FutureBase() = default;
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

class PromiseBase {
  template <typename Output>
  friend class Awaitable;

 private:
  virtual auto set_waited(FutureBase *future) -> void = 0;
};

template <typename Output>
class Awaitable {
  friend class Future<Output>;

 public:
  [[nodiscard]] auto await_ready() const noexcept -> bool {
    return future_.self_ ? future_.self_.done() : false;
  }

  auto await_suspend(Handle<void> waiting_coroutine) noexcept -> bool {
    future_.waiting_ = waiting_coroutine;
    if (waiting_coroutine) {
      Handle<PromiseBase>::from_address(waiting_coroutine.address())
          .promise()
          .set_waited(&future_);
    }
    // async function's return type
    if (future_.self_) {
      return true;
    }

    // Future object
    auto res = future_.poll(waiting_coroutine);
    if constexpr (std::is_same_v<Output, void>) {
      return std::holds_alternative<Pending>(res);
    } else {
      if (std::holds_alternative<Pending>(res)) {
        return true;
      }
      future_.return_ = std::get<Ready<Output>>(std::move(res)).inner_;
      return false;
    }
  }

  auto await_resume() -> Output requires(!std::is_same_v<Output, void>) {
    if (future_.waiting_) {
      Handle<PromiseBase>::from_address(future_.waiting_.address())
          .promise()
          .set_waited(nullptr);
    }

    auto return_v = std::move(future_.return_);
    if (!return_v.has_value()) {
      std::rethrow_exception(std::current_exception());
    }
    return std::move(return_v.value());
  }

  auto await_resume() -> void requires(std::is_same_v<Output, void>) {
    if (future_.waiting_) {
      Handle<PromiseBase>::from_address(future_.waiting_.address())
          .promise()
          .set_waited(nullptr);
    }

    auto ptr = future_.self_ ? future_.exception_ptr_ : nullptr;
    if (ptr) {
      std::rethrow_exception(ptr);
    }
  }

 private : explicit Awaitable(Future<Output> &future) noexcept
     : future_(future) {}
  Future<Output> &future_;
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
  friend class Awaitable<Output>;

 public:
  class PromiseType;
  using promise_type = PromiseType;

  class PromiseType : public PromiseBase {
    friend class Future<Output>;

   public:
    auto get_return_object() -> Future<Output> {
      return Future(Handle<promise_type>::from_promise(*this));
    }

    auto initial_suspend() noexcept -> InitFuture { return {}; }

    auto final_suspend() noexcept -> FinalAwaitable {
      return {future_->waiting_};
    }

    auto unhandled_exception() -> void {}

    auto return_value(Output &&value) -> void {
      future_->return_ = std::forward<Output>(value);
    }

   private:
    auto set_waited(FutureBase *future) -> void override {
      future_->waited_ = future;
    }

    Future<Output> *future_;
  };

  auto operator co_await() -> Awaitable<Output> { return Awaitable(*this); }

  [[nodiscard]] virtual auto poll(Handle<void> self) -> Poll<Output> {
    return Pending();
  }

  auto poll_wrapper() -> bool override {
    auto result = poll(waiting_);
    if (std::holds_alternative<Pending>(result)) {
      return false;
    }
    return_ = std::get<Ready<Output>>(std::move(result)).inner_;
    return true;
  }

  inline auto get_handle() -> Handle<void> override {
    return self_ == nullptr ? Handle<void>::from_address(waiting_.address())
                            : self_;
  }

  auto pending_future() -> FutureBase * override {
    return waited_ != nullptr ? waited_->pending_future() : this;
  }

  // co_await Future object should pass nullptr
  explicit Future(Handle<promise_type> self) : self_(self) {
    if (self_) {
      self_.promise().future_ = this;
    }
  }

  Future(const Future<Output> &future) = delete;

  Future(Future<Output> &&future) noexcept { *this = std::move(future); }

  auto operator=(const Future<Output> &future) -> Future<Output> & = delete;

  auto operator=(Future<Output> &&future) noexcept -> Future<Output> & {
    return_ = std::move(future.return_);
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

  ~Future() override {
    if (self_) {
      self_.destroy();
    }
  }

 private:
  std::optional<Output> return_{};
  Handle<promise_type> self_;
  Handle<void> waiting_;
  FutureBase *waited_{};
};

template <>
class Future<void> : public FutureBase {
  friend class Awaitable<void>;

 public:
  class PromiseType;
  using promise_type = PromiseType;

  class PromiseType : public PromiseBase {
    friend class Future<void>;

   public:
    auto get_return_object() -> Future<void>;

    auto initial_suspend() noexcept -> InitFuture;

    auto final_suspend() noexcept -> FinalAwaitable;

    auto unhandled_exception() -> void;

    auto return_void() -> void;

   private:
    auto set_waited(FutureBase *future) -> void override;

    Future<void> *future_{};
  };

  auto operator co_await() -> Awaitable<void>;

  [[nodiscard]] virtual auto poll(Handle<void> self) -> Poll<void>;

  [[nodiscard]] auto poll_wrapper() -> bool override;

  inline auto get_handle() -> Handle<void> override {
    return self_ == nullptr ? Handle<void>::from_address(waiting_.address())
                            : self_;
  }

  auto pending_future() -> FutureBase * override;

  explicit Future(Handle<promise_type> self);

  Future(const Future<void> &future) = delete;

  Future(Future<void> &&future) noexcept;

  auto operator=(const Future<void> &future) -> Future<void> & = delete;

  auto operator=(Future<void> &&future) noexcept -> Future<void> &;

  ~Future() override;

 private:
  std::exception_ptr exception_ptr_;
  Handle<promise_type> self_;
  Handle<void> waiting_;
  FutureBase *waited_{};
};
}  // namespace xyco::runtime

#endif  // XYCO_RUNTIME_FUTURE_H_