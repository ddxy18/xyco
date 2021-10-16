#ifndef XYCO_RUNTIME_ASYNC_H_
#define XYCO_RUNTIME_ASYNC_H_

#include <gsl/pointers>

#include "future.h"
#include "poll.h"
#include "runtime.h"

namespace runtime {
template <typename Return>
class AsyncFuture : public runtime::Future<Return> {
 public:
  [[nodiscard]] auto poll(runtime::Handle<void> self)
      -> runtime::Poll<Return> override {
    if (!ready_) {
      ready_ = true;
      auto res = RuntimeCtx::get_ctx()->blocking_handle()->Register(
          event_, runtime::Interest::All);
      return Pending();
    }

    gsl::owner<Return *> ret = gsl::owner<Return *>(
        std::get<AsyncFutureExtra>(event_.extra_).after_extra_);
    auto result = std::move(*ret);
    delete ret;

    return Ready<Return>{result};
  }

  template <typename Fn>
  explicit AsyncFuture(Fn &&f) requires(std::is_invocable_r_v<Return, Fn>)
      : Future<Return>(nullptr),
        f_([&]() {
          auto extra = AsyncFutureExtra{.after_extra_ = new Return(f())};
          event_.extra_ = extra;
        }),
        ready_(false),
        event_(runtime::Event{
            .future_ = this, .extra_ = AsyncFutureExtra{.before_extra_ = f_}}) {
  }

  AsyncFuture(const AsyncFuture<Return> &) = delete;

  AsyncFuture(AsyncFuture<Return> &&) = delete;

  auto operator=(const AsyncFuture<Return> &) -> AsyncFuture<Return> & = delete;

  auto operator=(AsyncFuture<Return> &&) -> AsyncFuture<Return> & = delete;

  ~AsyncFuture() = default;

 private:
  bool ready_;
  std::function<void()> f_;
  runtime::Event event_;
};
}  // namespace runtime

#endif  // XYCO_RUNTIME_ASYNC_H_