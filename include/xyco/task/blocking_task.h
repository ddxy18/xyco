#ifndef XYCO_TASK_BLOCKING_TASK_H_
#define XYCO_TASK_BLOCKING_TASK_H_

#include <gsl/pointers>
#include <type_traits>

#include "xyco/runtime/future.h"
#include "xyco/runtime/runtime_ctx.h"
#include "xyco/task/registry.h"

namespace xyco::task {
template <typename Fn>
class BlockingTask : public runtime::Future<std::invoke_result_t<Fn>> {
  using ReturnType = std::invoke_result_t<Fn>;

 public:
  [[nodiscard]] auto poll(runtime::Handle<void> self)
      -> runtime::Poll<ReturnType> override {
    if (!ready_) {
      ready_ = true;
      runtime::RuntimeCtx::get_ctx()->driver().Register<BlockingRegistry>(
          event_);
      return runtime::Pending();
    }

    gsl::owner<ReturnType *> ret = gsl::owner<ReturnType *>(
        dynamic_cast<BlockingExtra *>(event_->extra_.get())->after_extra_);
    auto result = std::move(*ret);
    runtime::RuntimeCtx::get_ctx()->driver().deregister<BlockingRegistry>(
        event_);
    delete ret;

    return runtime::Ready<ReturnType>{std::move(result)};
  }

  explicit BlockingTask(Fn &&function)
      : runtime::Future<ReturnType>(nullptr),
        f_([&]() {
          dynamic_cast<BlockingExtra *>(event_->extra_.get())->after_extra_ =
              gsl::owner<ReturnType *>(new ReturnType(function()));
        }),
        event_(std::make_shared<runtime::Event>(xyco::runtime::Event{
            .future_ = this, .extra_ = std::make_unique<BlockingExtra>(f_)})) {}

  BlockingTask(const BlockingTask<Fn> &) = delete;

  BlockingTask(BlockingTask<Fn> &&) = delete;

  auto operator=(const BlockingTask<Fn> &) -> BlockingTask<Fn> & = delete;

  auto operator=(BlockingTask<Fn> &&) -> BlockingTask<Fn> & = delete;

  virtual ~BlockingTask() = default;

 private:
  bool ready_{};
  std::function<void()> f_;
  std::shared_ptr<runtime::Event> event_;
};
}  // namespace xyco::task

#endif  // XYCO_TASK_BLOCKING_TASK_H_
