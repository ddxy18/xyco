#ifndef XYCO_RUNTIME_UTILS_SELECT_H
#define XYCO_RUNTIME_UTILS_SELECT_H

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
          future_wrapper<T1, 1>(std::move(futures_.first)),
          future_wrapper<T2, 2>(std::move(futures_.second)));
      wrappers_ = {wrapper1.get_handle(), wrapper2.get_handle()};
      RuntimeCtx::get_ctx()->spawn(std::move(wrapper1));
      RuntimeCtx::get_ctx()->spawn(std::move(wrapper2));
      return Pending();
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
      return Ready<CoOutput>{
          CoOutput(std::in_place_index<0>, std::get<1>(result_))};
    }

    {
      std::scoped_lock<std::mutex> first_lock(ended_mutex_.second);
      if (!ended_.first) {
        wrappers_.first.promise().future()->cancel();
      }
    }
    return Ready<CoOutput>{
        CoOutput(std::in_place_index<1>, std::get<2>(result_))};
  }

  SelectFuture(Future<T1> &&future1, Future<T2> &&future2)
      : Future<CoOutput>(nullptr),
        ready_(false),
        futures_(std::move(future1), std::move(future2)) {}

 private:
  template <typename T, int Index>
  auto future_wrapper(Future<T> future) -> Future<void> {
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
        RuntimeCtx::get_ctx()->register_future(this);
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

  bool ready_;

  std::mutex result_mutex_;
  std::variant<std::monostate, TypeWrapper<T1>, TypeWrapper<T2>> result_;

  std::pair<Future<T1>, Future<T2>> futures_;
  std::pair<Handle<PromiseBase>, Handle<PromiseBase>> wrappers_;

  std::pair<std::mutex, std::mutex> ended_mutex_;
  std::pair<bool, bool> ended_;
};

template <typename T1, typename T2>
auto select(Future<T1> future1, Future<T2> future2)
    -> Future<std::variant<TypeWrapper<T1>, TypeWrapper<T2>>> {
  co_return co_await SelectFuture<T1, T2>(std::move(future1),
                                          std::move(future2));
}
}  // namespace xyco::runtime

#endif  // XYCO_RUNTIME_UTILS_SELECT_H