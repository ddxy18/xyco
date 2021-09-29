#ifndef XYWEBSERVER_EVENT_RUNTIME_ASYNC_H_
#define XYWEBSERVER_EVENT_RUNTIME_ASYNC_H_

#include <gsl/pointers>

#include "future.h"
#include "poll.h"
#include "runtime_base.h"

namespace runtime {
template <typename Output>
class AsyncFuture : public runtime::Future<Output> {
 public:
  explicit AsyncFuture(std::function<Output()> f)
      : Future<Output>(nullptr),
        f_(std::function<void()>([&]() {
          auto res = f();
          event_.after_extra_ =
              static_cast<gsl::owner<Output *>>(new Output(res));
          return;
        })),
        ready_(false),
        event_(reactor::Event{reactor::Interest::All, -1, this, nullptr,
                              nullptr}) {
    event_.before_extra_ = &f_;
  }

  [[nodiscard]] auto poll(runtime::Handle<void> self)
      -> runtime::Poll<Output> override {
    if (!ready_) {
      ready_ = true;
      auto res = RuntimeCtx::get_ctx()->blocking_handle()->registry()->Register(
          &event_);
      return Pending();
    }
    return Ready<Output>{*static_cast<Output *>(event_.after_extra_)};
  }

 private:
  bool ready_;
  reactor::Event event_;
  std::function<void()> f_;
};
}  // namespace runtime

#endif  // XYWEBSERVER_EVENT_RUNTIME_ASYNC_H_