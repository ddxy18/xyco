#ifndef XYCO_TASK_JOIN_H_
#define XYCO_TASK_JOIN_H_

#include <mutex>

#include "xyco/runtime/runtime.h"
#include "xyco/runtime/runtime_ctx.h"
#include "xyco/task/type_wrapper.h"

namespace xyco::task {
template <typename T1, typename T2>
class JoinFuture
    : public runtime::Future<std::pair<TypeWrapper<T1>, TypeWrapper<T2>>> {
  using CoOutput = std::pair<TypeWrapper<T1>, TypeWrapper<T2>>;

 public:
  auto poll([[maybe_unused]] runtime::Handle<void> self)
      -> runtime::Poll<CoOutput> override {
    if (!ready_) {
      ready_ = true;
      runtime::RuntimeCtx::get_ctx()->get_runtime()->spawn(
          future_wrapper<T1, 0>(std::move(future1_)));
      runtime::RuntimeCtx::get_ctx()->get_runtime()->spawn(
          future_wrapper<T2, 1>(std::move(future2_)));
      return runtime::Pending();
    }

    return runtime::Ready<CoOutput>{CoOutput(*result_.first, *result_.second)};
  }

  JoinFuture(runtime::Future<T1> &&future1, runtime::Future<T2> &&future2)
      : runtime::Future<CoOutput>(nullptr),
        future1_(std::move(future1)),
        future2_(std::move(future2)),
        result_({}, {}) {}

 private:
  template <typename T, int Index>
  auto future_wrapper(runtime::Future<T> future) -> runtime::Future<void> {
    if constexpr (!std::is_same_v<T, void>) {
      auto result = co_await future;
      if constexpr (Index == 0) {
        result_.first = TypeWrapper<T>{result};
      } else {
        result_.second = TypeWrapper<T>{result};
      }
    } else {
      co_await future;
      if constexpr (Index == 0) {
        result_.first = TypeWrapper<T>();
      } else {
        result_.second = TypeWrapper<T>();
      }
    }
    {
      std::scoped_lock<std::mutex> lock_guard(mutex_);
      if (!registered_ && result_.first && result_.second) {
        registered_ = true;
        runtime::RuntimeCtx::get_ctx()->register_future(this);
      }
    }
  }

  bool ready_{};
  std::pair<std::optional<TypeWrapper<T1>>, std::optional<TypeWrapper<T2>>>
      result_;
  std::mutex mutex_;
  bool registered_{};
  runtime::Future<T1> future1_;
  runtime::Future<T2> future2_;
};

template <typename T1, typename T2>
auto join(runtime::Future<T1> future1, runtime::Future<T2> future2)
    -> runtime::Future<std::pair<TypeWrapper<T1>, TypeWrapper<T2>>> {
  co_return co_await JoinFuture<T1, T2>(std::move(future1), std::move(future2));
}
}  // namespace xyco::task

#endif  // XYCO_TASK_JOIN_H_
