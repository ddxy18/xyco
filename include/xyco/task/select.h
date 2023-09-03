#ifndef XYCO_TASK_SELECT_H_
#define XYCO_TASK_SELECT_H_

#include <shared_mutex>

#include "xyco/runtime/runtime.h"
#include "xyco/runtime/runtime_ctx.h"

namespace xyco::task {
template <typename T>
using Branch = std::tuple<runtime::Future<T>,
                          std::optional<std::expected<T, std::exception_ptr>>>;

template <typename... T>
class SelectFuture;

template <typename... T>
class BranchShared {
 public:
  BranchShared(SelectFuture<T...> *self, runtime::Future<T> &&...future)
      : self_(self),
        branches_({std::make_tuple(std::move(future), std::optional<T>())...}) {
  }

  bool registered_{};
  SelectFuture<T...> *self_;

  std::shared_mutex branch_mutex_;
  std::tuple<Branch<T>...> branches_;
};

template <typename... T>
class SelectFuture : public runtime::Future<std::tuple<std::optional<T>...>> {
  using CoOutput = std::tuple<std::optional<T>...>;

 public:
  auto poll([[maybe_unused]] runtime::Handle<void> self)
      -> runtime::Poll<CoOutput> override {
    if (!ready_) {
      ready_ = true;
      std::apply(
          [this](auto &...branch) {
            (runtime::RuntimeCtx::get_ctx()->get_runtime()->spawn(
                 run_single_branch(branch, branch_shared_)),
             ...);
          },
          branch_shared_->branches_);

      return runtime::Pending();
    }

    std::shared_lock<std::shared_mutex> guard(branch_shared_->branch_mutex_);
    auto result = std::apply(
        [](auto &...branch) {
          auto cancel = [](auto handle, auto result) {
            using ST =
                std::remove_reference_t<decltype(result.value().value())>;

            if (!result) {
              handle.promise().future()->cancel();
              return std::optional<ST>();
            }
            if (!result.value()) {
              std::rethrow_exception(result.value().error());
            }
            return std::optional<ST>(result.value().value());
          };
          return CoOutput(
              cancel(std::get<0>(branch).get_handle(), std::get<1>(branch))...);
        },
        branch_shared_->branches_);
    return runtime::Ready<CoOutput>{result};
  }

  SelectFuture(runtime::Future<T> &&...future)
      : runtime::Future<CoOutput>(nullptr),
        branch_shared_(
            std::make_shared<BranchShared<T...>>(this, std::move(future)...)) {}

  ~SelectFuture() {
    if (branch_shared_) {
      branch_shared_->self_ = nullptr;
    }
  }

  SelectFuture(const SelectFuture<T...> &future) = delete;

  SelectFuture(SelectFuture<T...> &&future) noexcept {
    *this = std::move(future);
  }

  auto operator=(const SelectFuture<T...> &future)
      -> SelectFuture<T...> & = delete;

  auto operator=(SelectFuture<T...> &&future) noexcept -> SelectFuture<T...> & {
    ready_ = future.ready_;
    branch_shared_ = std::move(future.branch_shared_);
  }

 private:
  template <typename ST>
  // NOLINTNEXTLINE(cppcoreguidelines-avoid-reference-coroutine-parameters)
  static auto run_single_branch(Branch<ST> &branch,
                                std::shared_ptr<BranchShared<T...>> branches)
      -> runtime::Future<void> {
    std::unique_lock<std::shared_mutex> guard{branches->branch_mutex_,
                                              std::defer_lock};
    try {
      auto result = co_await std::get<0>(branch);
      guard.lock();
      std::get<1>(branch) = result;
    } catch (runtime::CancelException e) {
      // Only thrown when one of other branches has finished, so we could ignore
      // it safely without worrying about dangling `select`.
      co_return;
    } catch (...) {
      guard.lock();
      std::get<1>(branch) = std::unexpected(std::current_exception());
    }

    if (!branches->registered_) {
      if (branches->self_) {
        runtime::RuntimeCtx::get_ctx()->register_future(branches->self_);
        branches->registered_ = true;
      }
    }
  }

  bool ready_{};

  // `select` returns once any branch completes, which means that the
  // `SelectFuture` instance may be destructed before some branches complete. So
  // we define this shared field passed to every branch to extend the lifetime
  // until all branches finished.
  std::shared_ptr<BranchShared<T...>> branch_shared_;
};

// Waits until the completion of at least one future.
// Uncaught exception from a future is also marked as the completion of the
// branch. `T` cannot contain `void` and the recommended alternative here is
// `std::nullptr_t`.
//
// Return value:
// A tuple containing results of every branch. At least one of these
// `std::optional` has a value.
//
// Exceptions:
// If any one of the futures throws an uncaught exception before `select`
// returns, it finally throws the first exception in a left to right sequence.
template <typename... T>
auto select(runtime::Future<T>... future)
    -> runtime::Future<std::tuple<std::optional<T>...>> {
  if (sizeof...(T) == 0) {
    co_return {};
  }

  co_return co_await SelectFuture<T...>(std::move(future)...);
}
}  // namespace xyco::task

#endif  // XYCO_TASK_SELECT_H_
