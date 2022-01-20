#ifndef XYCO_TIME_SLEEP_H
#define XYCO_TIME_SLEEP_H

#include <chrono>

#include "runtime/runtime.h"
#include "time/driver.h"

namespace xyco::time {
template <typename Rep, typename Ratio>
auto sleep(std::chrono::duration<Rep, Ratio> duration)
    -> runtime::Future<void> {
  class Future : public runtime::Future<void> {
   public:
    explicit Future(std::chrono::duration<Rep, Ratio> duration)
        : runtime::Future<void>(nullptr),
          duration_(duration),
          event_(runtime::Event{.extra_ = &extra_}) {}

    auto poll(runtime::Handle<void> self) -> runtime::Poll<void> override {
      if (!ready_) {
        ready_ = true;
        extra_.expire_time_ = std::chrono::system_clock::now() + duration_;
        event_.future_ = this;
        runtime::RuntimeCtx::get_ctx()
            ->driver()
            .handle<TimeRegistry>()
            ->Register(event_)
            .unwrap();

        return runtime::Pending();
      }

      return runtime::Ready<void>{};
    }

   private:
    bool ready_{};
    std::chrono::milliseconds duration_;
    time::TimeExtra extra_;
    runtime::Event event_;
  };

  co_await Future(duration);
}
}  // namespace xyco::time

#endif  // XYCO_TIME_SLEEP_H