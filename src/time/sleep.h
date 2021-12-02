#ifndef XYCO_TIME_SLEEP_H
#define XYCO_TIME_SLEEP_H

#include <chrono>

#include "runtime/runtime.h"

namespace xyco::time {
template <typename Rep, typename Ratio>
auto sleep(std::chrono::duration<Rep, Ratio> duration)
    -> runtime::Future<void> {
  class Future : public runtime::Future<void> {
   public:
    explicit Future(std::chrono::duration<Rep, Ratio> duration)
        : runtime::Future<void>(nullptr), duration_(duration) {}

    auto poll(runtime::Handle<void> self) -> runtime::Poll<void> override {
      if (!ready_) {
        ready_ = true;
        event_ = runtime::Event{
            .future_ = this,
            .extra_ = runtime::TimeExtra{
                .expire_time_ = std::chrono::system_clock::now() + duration_}};
        runtime::RuntimeCtx::get_ctx()
            ->time_handle()
            ->Register(event_)
            .unwrap();

        return runtime::Pending();
      }

      return runtime::Ready<void>{};
    }

   private:
    bool ready_{};
    std::chrono::milliseconds duration_;
    runtime::Event event_;
  };

  co_await Future(duration);
}
}  // namespace xyco::time

#endif  // XYCO_TIME_SLEEP_H