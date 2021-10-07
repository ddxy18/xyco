#ifndef XYWEBSERVER_EVENT_RUNTIME_ASYNC_H_
#define XYWEBSERVER_EVENT_RUNTIME_ASYNC_H_

#include <gsl/pointers>

#include "future.h"
#include "poll.h"
#include "runtime_base.h"

namespace runtime {
template <typename Return>
class AsyncFuture : public runtime::Future<Return> {
 public:
  [[nodiscard]] auto poll(runtime::Handle<void> self)
      -> runtime::Poll<Return> override {
    if (!ready_) {
      ready_ = true;
      auto res = RuntimeCtx::get_ctx()->blocking_handle()->registry()->Register(
          &event_);
      return Pending();
    }
    return Ready<Return>{*static_cast<Return *>(event_.after_extra_)};
  }

  template <typename Fn>
  requires(std::is_invocable_r_v<Return, Fn>) explicit AsyncFuture(Fn &&f)
      : Future<Return>(nullptr),
        f_([&]() {
          event_.after_extra_ = gsl::owner<Return *>((new Return(f())));
        }),
        ready_(false),
        event_(reactor::Event{reactor::Interest::All, -1, this, f_, nullptr}) {}

  AsyncFuture(const AsyncFuture<Return> &) = delete;

  AsyncFuture(AsyncFuture<Return> &&) = delete;

  auto operator=(const AsyncFuture<Return> &) -> AsyncFuture<Return> & = delete;

  auto operator=(AsyncFuture<Return> &&) -> AsyncFuture<Return> & = delete;

  ~AsyncFuture() = default;

 private:
  bool ready_;
  std::function<void()> f_;
  reactor::Event event_;
};
}  // namespace runtime

#endif  // XYWEBSERVER_EVENT_RUNTIME_ASYNC_H_