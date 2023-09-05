#ifndef XYCO_TIME_TIMEOUT_H_
#define XYCO_TIME_TIMEOUT_H_

#include "xyco/task/select.h"
#include "xyco/time/sleep.h"

namespace xyco::time {
template <typename T, typename Rep, typename Ratio>
auto timeout(std::chrono::duration<Rep, Ratio> duration,
             runtime::Future<T> future)
    -> runtime::Future<std::expected<T, std::nullptr_t>> {
  auto wrap_sleep = [](std::chrono::duration<Rep, Ratio> duration)
      -> runtime::Future<std::nullptr_t> {
    co_await sleep(duration);
    co_return nullptr;
  };

  if constexpr (std::is_same_v<T, void>) {
    auto wrap_future =
        [](runtime::Future<T> future) -> runtime::Future<std::nullptr_t> {
      co_await future;
      co_return nullptr;
    };
    auto [result, sleep_result] = co_await task::select(
        wrap_future(std::move(future)), wrap_sleep(duration));
    if (sleep_result) {
      co_return std::unexpected(nullptr);
    }
    co_return {};
  } else {
    auto [result, sleep_result] =
        co_await task::select(std::move(future), wrap_sleep(duration));
    if (sleep_result) {
      co_return std::unexpected(nullptr);
    }
    // NOLINTNEXTLINE(bugprone-unchecked-optional-access)
    co_return result.value();
  }
}
}  // namespace xyco::time

#endif  // XYCO_TIME_TIMEOUT_H_
