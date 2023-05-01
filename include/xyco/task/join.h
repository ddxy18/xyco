#ifndef XYCO_TASK_JOIN_H_
#define XYCO_TASK_JOIN_H_

#include <expected>
#include <mutex>

#include "xyco/runtime/runtime.h"
#include "xyco/runtime/runtime_ctx.h"

namespace xyco::task {
template <typename... T>
class JoinFuture : public runtime::Future<std::tuple<T...>> {
  using CoOutput = std::tuple<T...>;

 public:
  auto poll([[maybe_unused]] runtime::Handle<void> self)
      -> runtime::Poll<CoOutput> override {
    if (!ready_) {
      ready_ = true;
      std::apply(
          [this](auto &...branch) {
            (runtime::RuntimeCtx::get_ctx()->get_runtime()->spawn(
                 this->run_single_branch<T>(branch)),
             ...);
          },
          branches_);

      return runtime::Pending();
    }

    auto unwrapped_result = std::apply(
        [](auto &&...branch) {
          auto extract_result = [](auto result) {
            if (!result.has_value()) {
              std::rethrow_exception(result.error());
            }
            return result.value();
          };
          return CoOutput(extract_result(branch.second.value())...);
        },
        branches_);
    return runtime::Ready<CoOutput>{unwrapped_result};
  }

  JoinFuture(runtime::Future<T> &&...future)
      : runtime::Future<CoOutput>(nullptr),
        branches_(std::make_pair(std::move(future), std::optional<T>())...) {}

 private:
  template <typename ST>
  auto run_single_branch(
      // NOLINTNEXTLINE(cppcoreguidelines-avoid-reference-coroutine-parameters)
      std::pair<runtime::Future<ST>,
                std::optional<std::expected<ST, std::exception_ptr>>> &branch)
      -> runtime::Future<void> {
    try {
      branch.second = co_await branch.first;
    } catch (...) {
      branch.second = std::unexpected(std::current_exception());
    }

    {
      std::scoped_lock<std::mutex> lock_guard(branch_mutex_);
      if (registered_) {
        co_return;
      }

      auto all_completed = std::apply(
          // NOLINTNEXTLINE(cppcoreguidelines-avoid-reference-coroutine-parameters)
          [](auto &...branch) {
            auto branch_state_array =
                std::array<bool, sizeof...(T)>{branch.second.has_value()...};
            return std::all_of(branch_state_array.begin(),
                               branch_state_array.end(),
                               [](auto completed) { return completed; });
          },
          branches_);
      if (all_completed) {
        registered_ = true;
        runtime::RuntimeCtx::get_ctx()->register_future(this);
      }
    }
  }

  bool ready_{};
  std::mutex branch_mutex_;
  bool registered_{};
  std::tuple<std::pair<runtime::Future<T>,
                       std::optional<std::expected<T, std::exception_ptr>>>...>
      branches_;
};

// Waits until completion of all futures.
// Every future is independent from others. So crash of one future will not stop
// others and `join`. `T` cannot contain `void` and the recommended alternative
// here is `std::nullptr_t`.
//
// Exceptions:
// If any one of the futures throws an uncaught exception, `join` finally throws
// the first exception in a left to right sequence.
template <typename... T>
auto join(runtime::Future<T>... future) -> runtime::Future<std::tuple<T...>> {
  if (sizeof...(T) == 0) {
    co_return {};
  }

  co_return co_await JoinFuture<T...>(std::move(future)...);
}
}  // namespace xyco::task

#endif  // XYCO_TASK_JOIN_H_
