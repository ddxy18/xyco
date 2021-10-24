#ifndef XYCO_RUNTIME_UTILS_SELECT_H
#define XYCO_RUNTIME_UTILS_SELECT_H

#include <gsl/pointers>
#include <utility>

#include "runtime/runtime.h"
#include "type_wrapper.h"

namespace xyco::runtime {
template <typename T1, typename T2>
class SelectFuture
    : public Future<std::variant<TypeWrapper<T1>, TypeWrapper<T2>>> {
  using CoOutput = std::variant<TypeWrapper<T1>, TypeWrapper<T2>>;

 public:
  auto poll(Handle<void> self) -> Poll<CoOutput> override {
    if (!ready_) {
      ready_ = true;
      auto [wrapper1, wrapper2] = std::pair<Future<void>, Future<void>>(
          future_wrapper<T1, 0>(std::move(futures_.first)),
          future_wrapper<T2, 1>(std::move(futures_.second)));
      wrappers_ = {wrapper1.get_handle(), wrapper2.get_handle()};
      RuntimeCtx::get_ctx()->spawn(std::move(wrapper1));
      RuntimeCtx::get_ctx()->spawn(std::move(wrapper2));
      return Pending();
    }

    if (result_.index() == 0) {
      RuntimeCtx::get_ctx()->cancel_future(wrappers_.second);
      return Ready<CoOutput>{
          CoOutput(std::in_place_index<0>, std::get<0>(result_))};
    }

    RuntimeCtx::get_ctx()->cancel_future(wrappers_.first);
    return Ready<CoOutput>{
        CoOutput(std::in_place_index<1>, std::get<1>(result_))};
  }

  SelectFuture(Future<T1> &&future1, Future<T2> &&future2)
      : Future<CoOutput>(nullptr),
        ready_(false),
        result_(std::in_place_index<2>, true),
        futures_(std::move(future1), std::move(future2)) {}

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
  std::pair<Future<T1>, Future<T2>> futures_;
  std::pair<Handle<PromiseBase>, Handle<PromiseBase>> wrappers_;
};

template <typename T1, typename T2>
auto select(Future<T1> future1, Future<T2> future2)
    -> Future<std::variant<TypeWrapper<T1>, TypeWrapper<T2>>> {
  co_return co_await SelectFuture<T1, T2>(std::move(future1),
                                          std::move(future2));
}
}  // namespace xyco::runtime

#endif  // XYCO_RUNTIME_UTILS_SELECT_H