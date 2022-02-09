#ifndef XYCO_RUNTIME_FUTURE_H_
#define XYCO_RUNTIME_FUTURE_H_

#include <exception>
#include <experimental/coroutine>
#include <optional>
#include <variant>

namespace xyco::runtime {
class Runtime;
class FutureBase;
template <typename Output>
class Future;

template <typename T>
using Handle = std::experimental::coroutine_handle<T>;

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

class CancelException : public std::exception {};

class PromiseBase {
  template <typename Output>
  friend class Awaitable;

 public:
  virtual auto future() -> FutureBase * = 0;

  // NOLINTNEXTLINE(readability-convert-member-functions-to-static)
  auto initial_suspend() noexcept -> std::experimental::suspend_always {
    return {};
  }

 private:
  virtual auto set_waited(Handle<PromiseBase> future) -> void = 0;
};

class FutureBase {
 public:
  // Returns
  // true means the coroutine ends.
  [[nodiscard]] virtual auto poll_wrapper() -> bool = 0;

  // Returns
  // async function--self
  // Future object--the co_awaiter
  virtual auto get_handle() -> Handle<PromiseBase> = 0;

  virtual auto cancel() -> void = 0;

  FutureBase() = default;

  FutureBase(const FutureBase &future_base) = delete;

  FutureBase(FutureBase &&future_base) = delete;

  auto operator=(const FutureBase &future_base) -> const FutureBase & = delete;

  auto operator=(FutureBase &&future_base) -> const FutureBase & = delete;

  virtual ~FutureBase() = default;
};

class FinalAwaitable {
 public:
  FinalAwaitable(Handle<void> waiting_coroutine)
      : waiting_(waiting_coroutine) {}

  [[nodiscard]] auto await_ready() const noexcept -> bool {
    return waiting_ == nullptr;
  }

  auto await_suspend(Handle<void> waiting_coroutine) noexcept {
    return waiting_;
  }

  auto await_resume() noexcept -> void {}

 private:
  Handle<void> waiting_;
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
    Handle<PromiseBase>::from_address(waiting_coroutine.address())
        .promise()
        .set_waited(Handle<PromiseBase>::from_address(future_.self_.address()));

    // async function's return type
    if (future_.self_) {
      future_.self_.resume();
      auto ret = !future_.return_.has_value();
      if (ret) {
        future_.has_suspend_ = true;
      }
      return ret;
    }

    // Future object
    auto res = future_.poll(waiting_coroutine);
    auto ret = std::holds_alternative<Pending>(res);
    if (ret) {
      future_.has_suspend_ = true;
    }
    if constexpr (!std::is_same_v<Output, void>) {
      if (!ret) {
        future_.return_ = std::get<Ready<Output>>(std::move(res)).inner_;
      }
    }
    return ret;
  }

  auto await_resume() -> Output requires(!std::is_same_v<Output, void>) {
    if (future_.waiting_) {
      Handle<PromiseBase>::from_address(future_.waiting_.address())
          .promise()
          .set_waited(nullptr);
    }

    if (future_.cancelled_) {
      throw CancelException();
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

    if (future_.cancelled_) {
      throw CancelException();
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

    auto final_suspend() noexcept -> FinalAwaitable {
      return {future_->has_suspend_ ? future_->waiting_ : nullptr};
    }

    auto unhandled_exception() -> void {}

    auto return_value(Output &&value) -> void {
      future_->return_ = std::forward<Output>(value);
    }

    auto future() -> FutureBase * override { return future_; }

   private:
    auto set_waited(Handle<PromiseBase> future) -> void override {
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

  inline auto get_handle() -> Handle<PromiseBase> override {
    return self_ == nullptr
               ? Handle<PromiseBase>::from_address(waiting_.address())
               : Handle<PromiseBase>::from_address(self_.address());
  }

  auto cancel() -> void override {
    cancelled_ = true;
    if (waited_ != nullptr) {
      waited_.promise().future()->cancel();
    }
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
    has_suspend_ = future.has_suspend_;
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
    if (self_ && has_suspend_) {
      self_.destroy();
    }
  }

 private:
  std::optional<Output> return_{};
  Handle<promise_type> self_;
  bool has_suspend_{};
  Handle<void> waiting_;
  Handle<PromiseBase> waited_;

  bool cancelled_{};
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

    auto final_suspend() noexcept -> FinalAwaitable;

    auto unhandled_exception() -> void;

    auto return_void() -> void;

    auto future() -> FutureBase * override;

   private:
    auto set_waited(Handle<PromiseBase> future) -> void override;

    Future<void> *future_{};
  };

  auto operator co_await() -> Awaitable<void>;

  [[nodiscard]] virtual auto poll(Handle<void> self) -> Poll<void>;

  [[nodiscard]] auto poll_wrapper() -> bool override;

  inline auto get_handle() -> Handle<PromiseBase> override {
    return self_ == nullptr
               ? Handle<PromiseBase>::from_address(waiting_.address())
               : Handle<PromiseBase>::from_address(self_.address());
  }

  auto cancel() -> void override;

  explicit Future(Handle<promise_type> self);

  Future(const Future<void> &future) = delete;

  Future(Future<void> &&future) noexcept;

  auto operator=(const Future<void> &future) -> Future<void> & = delete;

  auto operator=(Future<void> &&future) noexcept -> Future<void> &;

  ~Future() override;

 private:
  std::optional<bool> return_{};
  std::exception_ptr exception_ptr_;
  Handle<promise_type> self_;
  bool has_suspend_{};
  Handle<void> waiting_;
  Handle<PromiseBase> waited_;

  bool cancelled_{};
};
}  // namespace xyco::runtime

#endif  // XYCO_RUNTIME_FUTURE_H_