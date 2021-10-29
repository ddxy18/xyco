#ifndef XYCO_RUNTIME_BLOCKING_H_
#define XYCO_RUNTIME_BLOCKING_H_

#include <mutex>
#include <queue>

#include "registry.h"

namespace xyco::runtime {
class BlockingRegistry;

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
  auto run(BlockingRegistry& blocking_registry) -> void;

  std::queue<Task> tasks_;
  std::mutex mutex_;
  std::condition_variable cv_;
  std::atomic_bool end_;
};

class BlockingPool {
 public:
  auto run(BlockingRegistry& blocking_registry) -> void;

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

class BlockingRegistry : public runtime::Registry {
  friend class BlockingWorker;

 public:
  explicit BlockingRegistry(uintptr_t woker_num);

  [[nodiscard]] auto Register(runtime::Event& ev, runtime::Interest interest)
      -> io::IoResult<void> override;

  [[nodiscard]] auto reregister(runtime::Event& ev, runtime::Interest interest)
      -> io::IoResult<void> override;

  [[nodiscard]] auto deregister(runtime::Event& ev, runtime::Interest interest)
      -> io::IoResult<void> override;

  [[nodiscard]] auto select(runtime::Events& events, int timeout)
      -> io::IoResult<void> override;

 private:
  runtime::Events events_;
  std::mutex mutex_;
  BlockingPool pool_;
};
}  // namespace xyco::runtime

#endif  // XYCO_RUNTIME_BLOCKING_H_