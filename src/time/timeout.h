#ifndef XYCO_TIME_TIMEOUT_H
#define XYCO_TIME_TIMEOUT_H

#include "runtime/utils/select.h"
#include "sleep.h"

namespace xyco::time {
// FIXME(dongxiaoyu): Cancel the future if timeout.
template <typename T, typename Rep, typename Ratio>
auto timeout(std::chrono::duration<Rep, Ratio> duration,
             runtime::Future<T> future) -> runtime::Future<Result<T, void>> {
  using CoOutput = Result<T, void>;

  auto result = co_await runtime::select(future, sleep(duration));

  if (result.index() == 1) {
    co_return CoOutput::err();
  }

  if constexpr (std::is_same_v<T, void>) {
    co_return CoOutput::ok();
  } else {
    co_return CoOutput::ok(std::get<0>(result).inner_);
  }
}
}  // namespace xyco::time

#endif  // XYCO_TIME_TIMEOUT_H