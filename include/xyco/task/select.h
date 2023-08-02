#ifndef XYCO_TASK_SELECT_H_
#define XYCO_TASK_SELECT_H_

#include "xyco/runtime/runtime.h"
#include "xyco/runtime/runtime_ctx.h"
#include "xyco/task/type_wrapper.h"

namespace xyco::task {
template <typename T1, typename T2>
class SelectFuture
    : public runtime::Future<std::variant<TypeWrapper<T1>, TypeWrapper<T2>>> {
  using CoOutput = std::variant<TypeWrapper<T1>, TypeWrapper<T2>>;

 public:
  auto poll(runtime::Handle<void> self) -> runtime::Poll<CoOutput> override {
    if (!ready_) {
      ready_ = true;
      auto [wrapper1, wrapper2] =
          std::pair<runtime::Future<void>, runtime::Future<void>>(
              future_wrapper<T1, 1>(std::move(futures_.first)),
              future_wrapper<T2, 2>(std::move(futures_.second)));
      wrappers_ = {wrapper1.get_handle(), wrapper2.get_handle()};
      runtime::RuntimeCtx::get_ctx()->get_runtime()->spawn(std::move(wrapper1));
      runtime::RuntimeCtx::get_ctx()->get_runtime()->spawn(std::move(wrapper2));
      return runtime::Pending();
    }

    decltype(result_.index()) index;
    {
      std::scoped_lock<std::mutex> result_lock(result_mutex_);
      index = result_.index();
    }
    if (index == 1) {
      {
        std::scoped_lock<std::mutex> second_lock(ended_mutex_.second);
        if (!ended_.second) {
          wrappers_.second.promise().future()->cancel();
        }
      }
      return runtime::Ready<CoOutput>{
          CoOutput(std::in_place_index<0>, std::get<1>(result_))};
    }

    {
      std::scoped_lock<std::mutex> first_lock(ended_mutex_.second);
      if (!ended_.first) {
        wrappers_.first.promise().future()->cancel();
      }
    }
    return runtime::Ready<CoOutput>{
        CoOutput(std::in_place_index<1>, std::get<2>(result_))};
  }

  SelectFuture(runtime::Future<T1> &&future1, runtime::Future<T2> &&future2)
      : runtime::Future<CoOutput>(nullptr),
        futures_(std::move(future1), std::move(future2)) {}

 private:
  template <typename T, int Index>
  auto future_wrapper(runtime::Future<T> future) -> runtime::Future<void> {
    TypeWrapper<T> result;
    if constexpr (std::is_same_v<T, void>) {
      co_await future;
    } else {
      result = TypeWrapper<T>{co_await future};
    }
    {
      std::scoped_lock<std::mutex> result_lock(result_mutex_);
      if (result_.index() == 0) {
        result_ =
            std::variant<std::monostate, TypeWrapper<T1>, TypeWrapper<T2>>(
                std::in_place_index<Index>, result);
        runtime::RuntimeCtx::get_ctx()->register_future(this);
      }
    }
    if constexpr (Index == 1) {
      {
        std::scoped_lock<std::mutex> first_lock(ended_mutex_.first);
        ended_.first = true;
      }
    } else {
      {
        std::scoped_lock<std::mutex> second_lock(ended_mutex_.first);
        ended_.second = true;
      }
    }
  }

  bool ready_{};

  std::mutex result_mutex_;
  std::variant<std::monostate, TypeWrapper<T1>, TypeWrapper<T2>> result_;

  std::pair<runtime::Future<T1>, runtime::Future<T2>> futures_;
  std::pair<runtime::Handle<runtime::PromiseBase>,
            runtime::Handle<runtime::PromiseBase>>
      wrappers_;

  std::pair<std::mutex, std::mutex> ended_mutex_;
  std::pair<bool, bool> ended_;
};

template <typename T1, typename T2>
auto select(runtime::Future<T1> future1, runtime::Future<T2> future2)
    -> runtime::Future<std::variant<TypeWrapper<T1>, TypeWrapper<T2>>> {
  co_return co_await SelectFuture<T1, T2>(std::move(future1),
                                          std::move(future2));
}
}  // namespace xyco::task

#endif  // XYCO_TASK_SELECT_H_
