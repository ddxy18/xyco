#ifndef XYCO_TIME_TIMEOUT_H_
#define XYCO_TIME_TIMEOUT_H_

#include "xyco/task/select.h"
#include "xyco/time/sleep.h"

namespace xyco::time {
template <typename T, typename Rep, typename Ratio>
auto timeout(std::chrono::duration<Rep, Ratio> duration,
             runtime::Future<T> future) -> runtime::Future<Result<T, void>> {
  using CoOutput = Result<T, void>;

  auto result = co_await task::select(std::move(future), sleep(duration));

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

#endif  // XYCO_TIME_TIMEOUT_H_
