#ifndef XYCO_TIME_SLEEP_H
#define XYCO_TIME_SLEEP_H

#include "clock.h"
#include "runtime/future.h"
#include "runtime/runtime_ctx.h"
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
          event_(std::make_shared<runtime::Event>(
              runtime::Event{.extra_ = std::make_unique<TimeExtra>()})) {}

    auto poll(runtime::Handle<void> self) -> runtime::Poll<void> override {
      if (!ready_) {
        ready_ = true;
        auto *extra = dynamic_cast<TimeExtra *>(event_->extra_.get());
        extra->expire_time_ = Clock::now() + duration_;
        event_->future_ = this;
        runtime::RuntimeCtx::get_ctx()->driver().Register<TimeRegistry>(event_);

        return runtime::Pending();
      }

      return runtime::Ready<void>();
    }

   private:
    bool ready_{};
    std::chrono::milliseconds duration_;
    std::shared_ptr<runtime::Event> event_;
  };

  co_await Future(duration);
}
}  // namespace xyco::time

#endif  // XYCO_TIME_SLEEP_H