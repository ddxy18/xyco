#ifndef XYCO_RUNTIME_UTILS_JOIN_H
#define XYCO_RUNTIME_UTILS_JOIN_H

#include <mutex>

#include "runtime/runtime.h"
#include "type_wrapper.h"

namespace xyco::runtime {
template <typename T1, typename T2>
class JoinFuture : public Future<std::pair<TypeWrapper<T1>, TypeWrapper<T2>>> {
  using CoOutput = std::pair<TypeWrapper<T1>, TypeWrapper<T2>>;

 public:
  auto poll(Handle<void> self) -> Poll<CoOutput> override {
    if (!ready_) {
      ready_ = true;
      RuntimeCtx::get_ctx()->spawn(future_wrapper<T1, 0>(std::move(future1_)));
      RuntimeCtx::get_ctx()->spawn(future_wrapper<T2, 1>(std::move(future2_)));
      return Pending();
    }

    return Ready<CoOutput>{
        CoOutput(result_.first.value(), result_.second.value())};
  }

  JoinFuture(Future<T1> &&future1, Future<T2> &&future2)
      : Future<CoOutput>(nullptr),
        future1_(std::move(future1)),
        future2_(std::move(future2)),
        result_({}, {}) {}

 private:
  template <typename T, int Index>
  auto future_wrapper(Future<T> future) -> Future<void> {
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
      if (!registered_ && result_.first.has_value() &&
          result_.second.has_value()) {
        registered_ = true;
        RuntimeCtx::get_ctx()->register_future(this);
      }
    }
  }

  bool ready_{};
  std::pair<std::optional<TypeWrapper<T1>>, std::optional<TypeWrapper<T2>>>
      result_;
  std::mutex mutex_;
  bool registered_{};
  Future<T1> &&future1_;
  Future<T2> &&future2_;
};

template <typename T1, typename T2>
auto join(Future<T1> future1, Future<T2> future2)
    -> Future<std::pair<TypeWrapper<T1>, TypeWrapper<T2>>> {
  co_return co_await JoinFuture<T1, T2>(std::move(future1), std::move(future2));
}
}  // namespace xyco::runtime

#endif  // XYCO_RUNTIME_UTILS_JOIN_H