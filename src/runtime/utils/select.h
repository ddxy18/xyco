#ifndef XYCO_RUNTIME_UTILS_SELECT_H
#define XYCO_RUNTIME_UTILS_SELECT_H

#include "runtime/runtime.h"

namespace xyco::runtime {
template <typename T>
class TypeWrapper {
 public:
  T inner_;
};

template <>
class TypeWrapper<void> {};

// FIXME(dongxiaoyu): cancel another future
template <typename T1, typename T2>
class SelectFuture
    : public Future<std::variant<TypeWrapper<T1>, TypeWrapper<T2>>> {
  using CoOutput = std::variant<TypeWrapper<T1>, TypeWrapper<T2>>;

 public:
  auto poll(Handle<void> self) -> Poll<CoOutput> override {
    if (!ready_) {
      ready_ = true;
      future_wrapper<T1, 0>(future1_);
      future_wrapper<T2, 1>(future2_);
      return Pending();
    }

    return result_.index() == 0 ? Ready<CoOutput>{std::get<0>(result_)}
                                : Ready<CoOutput>{std::get<1>(result_)};
  }

  SelectFuture(Future<T1> &future1, Future<T2> &future2)
      : Future<CoOutput>(nullptr),
        ready_(false),
        future1_(future1),
        future2_(future2),
        result_(std::in_place_index<2>, true) {}

 private:
  template <typename T, int Index>
  auto future_wrapper(Future<T> future) -> Future<void>
  requires(!std::is_same_v<T, void>) {
    auto result = co_await future;
    if (result_.index() == 2) {
      result_ = std::variant<TypeWrapper<T1>, TypeWrapper<T2>, bool>(
          std::in_place_index<Index>, TypeWrapper<T>{result});
      RuntimeCtx::get_ctx()->register_future(this);
    }
  }

  template <typename T, int Index>
  auto future_wrapper(Future<T> future) -> Future<void>
  requires(std::is_same_v<T, void>) {
    co_await future;
    if (result_.index() == 2) {
      result_ = std::variant<TypeWrapper<T1>, TypeWrapper<T2>, bool>(
          std::in_place_index<Index>, TypeWrapper<T>());
      RuntimeCtx::get_ctx()->register_future(this);
    }
  }

  bool ready_;
  std::variant<TypeWrapper<T1>, TypeWrapper<T2>, bool> result_;
  Future<T1> &future1_;
  Future<T2> &future2_;
};

template <typename T1, typename T2>
auto select(Future<T1> future1, Future<T2> future2)
    -> Future<std::variant<TypeWrapper<T1>, TypeWrapper<T2>>> {
  co_return co_await SelectFuture<T1, T2>(future1, future2);
}
}  // namespace xyco::runtime

#endif  // XYCO_RUNTIME_UTILS_SELECT_H