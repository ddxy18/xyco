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
      auto res =
          RuntimeCtx::get_ctx()->driver().handle<BlockingRegistry>()->Register(
              event_);
      return Pending();
    }

    gsl::owner<Return *> ret = gsl::owner<Return *>(extra_.after_extra_);
    auto result = std::move(*ret);
    delete ret;

    return Ready<Return>{std::move(result)};
  }

  template <typename Fn>
  explicit AsyncFuture(Fn &&f) requires(std::is_invocable_r_v<Return, Fn>)
      : Future<Return>(nullptr),
        f_([&]() {
          extra_.after_extra_ = gsl::owner<Return *>(new Return(f()));
        }),
        ready_(false),
        extra_(f_),
        event_(xyco::runtime::Event{.future_ = this, .extra_ = &extra_}) {}

  AsyncFuture(const AsyncFuture<Return> &) = delete;

  AsyncFuture(AsyncFuture<Return> &&) = delete;

  auto operator=(const AsyncFuture<Return> &) -> AsyncFuture<Return> & = delete;

  auto operator=(AsyncFuture<Return> &&) -> AsyncFuture<Return> & = delete;

  ~AsyncFuture() = default;

 private:
  bool ready_;
  std::function<void()> f_;
  AsyncFutureExtra extra_;
  Event event_;
};
}  // namespace xyco::runtime

#endif  // XYCO_RUNTIME_ASYNC_H_