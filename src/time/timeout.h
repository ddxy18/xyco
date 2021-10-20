#ifndef XYCO_TIME_TIMEOUT_H
#define XYCO_TIME_TIMEOUT_H

#include "sleep.h"

namespace xyco::time {
// FIXME(dongxiaoyu): Cancel the future if timeout. Return immediately when
// future completes.
template <typename T, typename Rep, typename Ratio>
auto timeout(std::chrono::duration<Rep, Ratio> duration,
             runtime::Future<T> future) -> runtime::Future<Result<T, void>> {
  using CoOutput = Result<T, void>;

  std::optional<T> return_value;
  auto future_wrapper =
      [&](runtime::Future<T> future) -> runtime::Future<void> {
    return_value = co_await future;
  };

  auto new_future = future_wrapper(future);

  if (return_value.has_value()) {
    co_return CoOutput::ok(return_value.value());
  }

  co_await sleep(duration);
  if (return_value.has_value()) {
    co_return CoOutput::ok(return_value.value());
  }

  co_return CoOutput::err();
}
}  // namespace xyco::time

#endif  // XYCO_TIME_TIMEOUT_H