#ifndef XYCO_RUNTIME_ASYNC_H_
#define XYCO_RUNTIME_ASYNC_H_

#include <gsl/pointers>

#include "future.h"
#include "poll.h"
#include "runtime.h"
#include "runtime/blocking.h"

namespace xyco::runtime {
template <typename Return>
class AsyncFuture : public Future<Return> {
 public:
  [[nodiscard]] auto poll(Handle<void> self) -> Poll<Return> override {
    if (!ready_) {
      ready_ = true;
      RuntimeCtx::get_ctx()->driver().Register<BlockingRegistry>(event_);
      return Pending();
    }

    gsl::owner<Return *> ret = gsl::owner<Return *>(
        dynamic_cast<AsyncFutureExtra *>(event_->extra_.get())->after_extra_);
    auto result = std::move(*ret);
    RuntimeCtx::get_ctx()->driver().deregister<BlockingRegistry>(event_);
    delete ret;

    return Ready<Return>{std::move(result)};
  }

  template <typename Fn>
  explicit AsyncFuture(Fn &&function)
    requires(std::is_invocable_r_v<Return, Fn>)
      : Future<Return>(nullptr),
        f_([&]() {
          dynamic_cast<AsyncFutureExtra *>(event_->extra_.get())->after_extra_ =
              gsl::owner<Return *>(new Return(function()));
        }),
        event_(std::make_shared<Event>(xyco::runtime::Event{
            .future_ = this,
            .extra_ = std::make_unique<AsyncFutureExtra>(f_)})) {}

  AsyncFuture(const AsyncFuture<Return> &) = delete;

  AsyncFuture(AsyncFuture<Return> &&) = delete;

  auto operator=(const AsyncFuture<Return> &) -> AsyncFuture<Return> & = delete;

  auto operator=(AsyncFuture<Return> &&) -> AsyncFuture<Return> & = delete;

  virtual ~AsyncFuture() = default;

 private:
  bool ready_{};
  std::function<void()> f_;
  std::shared_ptr<Event> event_;
};
}  // namespace xyco::runtime

#endif  // XYCO_RUNTIME_ASYNC_H_