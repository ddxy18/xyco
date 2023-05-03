#ifndef XYCO_RUNTIME_BLOCKING_H_
#define XYCO_RUNTIME_BLOCKING_H_

#include <mutex>
#include <queue>

#include "runtime/global_registry.h"

namespace xyco::runtime {
class BlockingRegistryImpl;

class AsyncFutureExtra : public Extra {
 public:
  [[nodiscard]] auto print() const -> std::string override;

  AsyncFutureExtra(std::function<void()> before_extra);

  std::function<void()> before_extra_;
  void* after_extra_{};
};

class Task {
 public:
  auto operator()() -> void;

  explicit Task(std::function<void()> task);

 private:
  std::function<void()> inner_;
};

class BlockingWorker {
  friend class BlockingPool;

 private:
  auto run(BlockingRegistryImpl& blocking_registry) -> void;

  std::queue<Task> tasks_;
  std::mutex mutex_;
  std::condition_variable cv_;
  std::atomic_bool end_;
};

class BlockingPool {
 public:
  auto run(BlockingRegistryImpl& blocking_registry) -> void;

  auto spawn(Task task) -> void;

  explicit BlockingPool(uintptr_t worker_num);

  BlockingPool(const BlockingPool& runtime) = delete;

  BlockingPool(BlockingPool&& runtime) = delete;

  auto operator=(const BlockingPool& runtime) -> BlockingPool& = delete;

  auto operator=(BlockingPool&& runtime) -> BlockingPool& = delete;

  ~BlockingPool();

 private:
  std::vector<BlockingWorker> workers_;
  std::vector<std::thread> worker_ctx_;
};

class BlockingRegistryImpl : public runtime::Registry {
  friend class BlockingWorker;

 public:
  explicit BlockingRegistryImpl(uintptr_t woker_num);

  [[nodiscard]] auto Register(std::shared_ptr<Event> event)
      -> utils::Result<void> override;

  [[nodiscard]] auto reregister(std::shared_ptr<Event> event)
      -> utils::Result<void> override;

  [[nodiscard]] auto deregister(std::shared_ptr<Event> event)
      -> utils::Result<void> override;

  [[nodiscard]] auto select(runtime::Events& events,
                            std::chrono::milliseconds timeout)
      -> utils::Result<void> override;

 private:
  runtime::Events events_;
  std::mutex mutex_;
  BlockingPool pool_;
};

using BlockingRegistry = GlobalRegistry<BlockingRegistryImpl>;
}  // namespace xyco::runtime

template <>
struct std::formatter<xyco::runtime::AsyncFutureExtra>
    : public std::formatter<std::string> {
  template <typename FormatContext>
  auto format(const xyco::runtime::AsyncFutureExtra& extra,
              FormatContext& ctx) const -> decltype(ctx.out()) {
    return std::format_to(ctx.out(), "AsyncFutureExtra{{}}");
  }
};

#endif  // XYCO_RUNTIME_BLOCKING_H_
