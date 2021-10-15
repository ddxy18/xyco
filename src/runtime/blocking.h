#ifndef XYCO_RUNTIME_BLOCKING_H_
#define XYCO_RUNTIME_BLOCKING_H_

#include <atomic>
#include <condition_variable>
#include <mutex>
#include <queue>
#include <thread>
#include <vector>

namespace xyco::runtime {
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
  auto run() -> void;

  std::queue<Task> tasks_;
  std::mutex mutex_;
  std::condition_variable cv_;
  std::atomic_bool end_;
};

class BlockingPool {
 public:
  auto run() -> void;

  auto spawn(Task task) -> void;

  explicit BlockingPool(uintptr_t worker_num);

  BlockingPool(const BlockingPool &runtime) = delete;

  BlockingPool(BlockingPool &&runtime) = delete;

  auto operator=(const BlockingPool &runtime) -> BlockingPool & = delete;

  auto operator=(BlockingPool &&runtime) -> BlockingPool & = delete;

  ~BlockingPool();

 private:
  std::vector<BlockingWorker> workers_;
  std::vector<std::thread> worker_ctx_;
};
}  // namespace xyco::runtime

#endif  // XYCO_RUNTIME_BLOCKING_H_