#ifndef XYCO_SYNC_MPSC_H
#define XYCO_SYNC_MPSC_H

#include <queue>

#include "runtime/runtime.h"

namespace xyco::sync::mpsc {
template <typename Value, size_t Size>
class Sender;

template <typename Value, size_t Size>
class Receiver;

template <typename Value, size_t Size>
auto channel() -> std::pair<Sender<Value, Size>, Receiver<Value, Size>>;

template <typename Value, size_t Size>
class Shared {
  friend class Sender<Value, Size>;
  friend class Receiver<Value, Size>;

 private:
  static constexpr char sender_closed = 1;
  static constexpr char receiver_closed = 2;

  std::atomic_char state_;

  std::mutex receiver_mutex_;
  runtime::FutureBase *receiver_{};

  std::mutex senders_mutex_;
  std::queue<runtime::FutureBase *> senders_;

  std::mutex queue_mutex_;
  std::queue<Value> queue_;
};

template <typename Value, size_t Size>
class Sender {
  friend auto channel<Value, Size>()
      -> std::pair<Sender<Value, Size>, Receiver<Value, Size>>;

 public:
  auto send(Value &&value) -> runtime::Future<Result<void, Value>> {
    using FutureReturn = Result<void, Value>;
    class Future : public runtime::Future<FutureReturn> {
     public:
      explicit Future(Sender<Value, Size> *self, Value &&value)
          : runtime::Future<FutureReturn>(nullptr),
            self_(self),
            value_(std::forward<Value>(value)) {}

      auto poll(runtime::Handle<void> self)
          -> runtime::Poll<FutureReturn> override {
        if (self_->shared_->state_ == Shared<Value, Size>::receiver_closed) {
          return runtime::Ready<FutureReturn>{
              FutureReturn::err(std::forward<Value>(value_))};
        }

        {
          std::scoped_lock<std::mutex> queue_guard(
              self_->shared_->queue_mutex_);
          if (self_->shared_->queue_.size() < Size) {
            self_->shared_->queue_.push(std::forward<Value>(value_));

            {
              std::scoped_lock<std::mutex> receiver_guard(
                  self_->shared_->receiver_mutex_);
              if (self_->shared_->receiver_ != nullptr) {
                runtime::RuntimeCtx::get_ctx()->register_future(
                    self_->shared_->receiver_);
              }
            }
            return runtime::Ready<FutureReturn>{FutureReturn::ok()};
          }
        }

        {
          std::scoped_lock<std::mutex> senders_guard(
              self_->shared_->senders_mutex_);
          self_->shared_->senders_.push(this);
        }
        return runtime::Pending();
      }

     private:
      Sender<Value, Size> *self_;
      Value value_;
    };

    co_return co_await Future(this, std::forward<Value>(value));
  }

  Sender(const Sender<Value, Size> &sender) = default;

  Sender(Sender<Value, Size> &&sender) noexcept = default;

  auto operator=(const Sender<Value, Size> &sender)
      -> Sender<Value, Size> & = default;

  auto operator=(Sender<Value, Size> &&sender) noexcept
      -> Sender<Value, Size> & = default;

  ~Sender() { close(); }

 private:
  auto close() -> void {
    if (shared_ != nullptr) {
      auto owner_num = shared_.use_count();
      if (shared_->state_ != Shared<Value, Size>::receiver_closed) {
        owner_num--;
      }
      if (owner_num == 1) {
        shared_->state_ = Shared<Value, Size>::sender_closed;
      }
    }
  }

  Sender(std::shared_ptr<Shared<Value, Size>> shared) : shared_(shared) {}

  std::shared_ptr<Shared<Value, Size>> shared_;
};

template <typename Value, size_t Size>
class Receiver {
  friend auto channel<Value, Size>()
      -> std::pair<Sender<Value, Size>, Receiver<Value, Size>>;

 public:
  auto receive() -> runtime::Future<std::optional<Value>> {
    using FutureReturn = std::optional<Value>;

    class Future : public runtime::Future<FutureReturn> {
     public:
      explicit Future(Receiver<Value, Size> *self)
          : runtime::Future<FutureReturn>(nullptr), self_(self) {}

      auto poll(runtime::Handle<void> self)
          -> runtime::Poll<FutureReturn> override {
        std::unique_lock<std::mutex> queue_guard(self_->shared_->queue_mutex_);
        if (!self_->shared_->queue_.empty()) {
          auto top = self_->shared_->queue_.front();
          self_->shared_->queue_.pop();
          queue_guard.unlock();

          {
            std::scoped_lock<std::mutex> senders_guard(
                self_->shared_->senders_mutex_);
            if (!self_->shared_->senders_.empty()) {
              auto sender = self_->shared_->senders_.front();
              self_->shared_->senders_.pop();
              runtime::RuntimeCtx::get_ctx()->register_future(sender);
            }
          }

          return runtime::Ready<FutureReturn>{top};
        }
        if (self_->shared_->state_ == Shared<Value, Size>::sender_closed) {
          return runtime::Ready<FutureReturn>{{}};
        }

        {
          std::scoped_lock<std::mutex> receiver_guard(
              self_->shared_->receiver_mutex_);
          self_->shared_->receiver_ = this;
        }
        return runtime::Pending();
      }

     private:
      Receiver<Value, Size> *self_;
    };

    co_return co_await Future(this);
  }

  Receiver(const Receiver<Value, Size> &sender) = default;

  Receiver(Receiver<Value, Size> &&sender) noexcept = default;

  auto operator=(const Receiver<Value, Size> &sender)
      -> Receiver<Value, Size> & = default;

  auto operator=(Receiver<Value, Size> &&sender) noexcept
      -> Receiver<Value, Size> & = default;

  ~Receiver() { close(); }

 private:
  auto close() -> void {
    if (shared_ != nullptr) {
      shared_->state_ = Shared<Value, Size>::receiver_closed;
    }
  }

  Receiver(std::shared_ptr<Shared<Value, Size>> shared) : shared_(shared) {}

  std::shared_ptr<Shared<Value, Size>> shared_;
};

template <typename Value, size_t Size>
auto channel() -> std::pair<Sender<Value, Size>, Receiver<Value, Size>> {
  auto shared = std::make_shared<Shared<Value, Size>>();
  return {Sender<Value, Size>(shared),
          Receiver<Value, Size>(std::move(shared))};
}
}  // namespace xyco::sync::mpsc

#endif  // XYCO_SYNC_MPSC_H