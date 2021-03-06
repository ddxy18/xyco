#ifndef XYCO_SYNC_ONESHOT_H
#define XYCO_SYNC_ONESHOT_H

#include "runtime/runtime.h"

namespace xyco::sync::oneshot {
template <typename Value>
class Sender;

template <typename Value>
class Receiver;

template <typename Value>
auto channel() -> std::pair<Sender<Value>, Receiver<Value>>;

template <typename Value>
class Shared {
  friend class Sender<Value>;
  friend class Receiver<Value>;

 private:
  static constexpr char sender_closed = 1;
  static constexpr char receiver_closed = 2;

  std::atomic_char state_{};
  std::optional<Value> value_;
  runtime::FutureBase *receiver_{};
};

template <typename Value>
class Sender {
  friend auto channel<Value>() -> std::pair<Sender<Value>, Receiver<Value>>;

 public:
  auto send(Value &&value) -> runtime::Future<Result<void, Value>> {
    if (shared_ == nullptr ||
        shared_->state_ == Shared<Value>::receiver_closed) {
      co_return Result<void, Value>::err(std::forward<Value>(value));
    }

    shared_->value_ = std::forward<Value>(value);
    if (shared_->receiver_ != nullptr) {
      runtime::RuntimeCtx::get_ctx()->register_future(shared_->receiver_);
    }
    close();

    co_return Result<void, Value>::ok();
  }

  Sender(const Sender<Value> &sender) = delete;

  Sender(Sender<Value> &&sender) noexcept = default;

  auto operator=(const Sender<Value> &sender) -> Sender<Value> & = delete;

  auto operator=(Sender<Value> &&sender) noexcept -> Sender<Value> & = default;

  ~Sender() { close(); }

 private:
  auto close() -> void {
    if (shared_ != nullptr) {
      shared_->state_ = Shared<Value>::sender_closed;
      shared_ = nullptr;
    }
  }

  Sender(std::shared_ptr<Shared<Value>> shared) : shared_(shared) {}

  std::shared_ptr<Shared<Value>> shared_;
};

template <typename Value>
class Receiver {
  friend auto channel<Value>() -> std::pair<Sender<Value>, Receiver<Value>>;

 public:
  auto receive() -> runtime::Future<Result<Value, void>> {
    class Future : public runtime::Future<Result<Value, void>> {
     public:
      explicit Future(Receiver<Value> *self)
          : runtime::Future<Result<Value, void>>(nullptr), self_(self) {}

      auto poll(runtime::Handle<void> self)
          -> runtime::Poll<Result<Value, void>> override {
        if (self_->shared_ == nullptr) {
          return runtime::Ready<Result<Value, void>>{
              Result<Value, void>::err()};
        }
        if (self_->shared_->value_.has_value()) {
          return runtime::Ready<Result<Value, void>>{
              Result<Value, void>::ok(self_->shared_->value_.value())};
        }
        if (self_->shared_->state_ == Shared<Value>::sender_closed) {
          return runtime::Ready<Result<Value, void>>{
              Result<Value, void>::err()};
        }
        self_->shared_->receiver_ = this;
        return runtime::Pending();
      }

     private:
      Receiver<Value> *self_;
    };

    auto result = co_await Future(this);
    close();
    co_return result;
  }

  Receiver(const Receiver<Value> &sender) = delete;

  Receiver(Receiver<Value> &&sender) noexcept = default;

  auto operator=(const Receiver<Value> &sender) -> Receiver<Value> & = delete;

  auto operator=(Receiver<Value> &&sender) noexcept
      -> Receiver<Value> & = default;

  ~Receiver() { close(); }

 private:
  auto close() -> void {
    if (shared_ != nullptr) {
      shared_->state_ = Shared<Value>::receiver_closed;
      shared_ = nullptr;
    }
  }

  Receiver(std::shared_ptr<Shared<Value>> shared) : shared_(shared) {}

  std::shared_ptr<Shared<Value>> shared_;
};

template <typename Value>
auto channel() -> std::pair<Sender<Value>, Receiver<Value>> {
  auto shared = std::make_shared<Shared<Value>>();
  return {Sender<Value>(shared), Receiver<Value>(std::move(shared))};
}

}  // namespace xyco::sync::oneshot

#endif  // XYCO_SYNC_ONESHOT_H