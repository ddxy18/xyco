#ifndef XYCO_RUNTIME_ASYNC_H_
#define XYCO_RUNTIME_ASYNC_H_

#include <gsl/pointers>
#include <type_traits>

#include "future.h"
#include "poll.h"
#include "runtime.h"
#include "runtime/blocking.h"

namespace xyco::runtime {
template <typename Fn>
class AsyncFuture : public Future<std::invoke_result_t<Fn>> {
  using ReturnType = std::invoke_result_t<Fn>;

 public:
  [[nodiscard]] auto poll(Handle<void> self) -> Poll<ReturnType> override {
    if (!ready_) {
      ready_ = true;
      RuntimeCtx::get_ctx()->driver().Register<BlockingRegistry>(event_);
      return Pending();
    }

    gsl::owner<ReturnType *> ret = gsl::owner<ReturnType *>(
        dynamic_cast<AsyncFutureExtra *>(event_->extra_.get())->after_extra_);
    auto result = std::move(*ret);
    RuntimeCtx::get_ctx()->driver().deregister<BlockingRegistry>(event_);
    delete ret;

    return Ready<ReturnType>{std::move(result)};
  }

  explicit AsyncFuture(Fn &&function)
      : Future<ReturnType>(nullptr),
        f_([&]() {
          dynamic_cast<AsyncFutureExtra *>(event_->extra_.get())->after_extra_ =
              gsl::owner<ReturnType *>(new ReturnType(function()));
        }),
        event_(std::make_shared<Event>(xyco::runtime::Event{
            .future_ = this,
            .extra_ = std::make_unique<AsyncFutureExtra>(f_)})) {}

  AsyncFuture(const AsyncFuture<Fn> &) = delete;

  AsyncFuture(AsyncFuture<Fn> &&) = delete;

  auto operator=(const AsyncFuture<Fn> &) -> AsyncFuture<Fn> & = delete;

  auto operator=(AsyncFuture<Fn> &&) -> AsyncFuture<Fn> & = delete;

  virtual ~AsyncFuture() = default;

 private:
  bool ready_{};
  std::function<void()> f_;
  std::shared_ptr<Event> event_;
};
}  // namespace xyco::runtime

#endif  // XYCO_RUNTIME_ASYNC_H_
