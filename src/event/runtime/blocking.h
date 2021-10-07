#ifndef XYWEBSERVER_EVENT_RUNTIME_BLOCKING_H_
#define XYWEBSERVER_EVENT_RUNTIME_BLOCKING_H_

#include <atomic>
#include <condition_variable>
#include <mutex>
#include <queue>
#include <thread>
#include <vector>

namespace blocking {
class Task {
 public:
  auto operator()() -> void;

  explicit Task(std::function<void()> task);

 private:
  std::function<void()> inner_;
};

class Worker {
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

  explicit BlockingPool(int worker_num);

  BlockingPool(const BlockingPool &runtime) = delete;

  BlockingPool(BlockingPool &&runtime) = delete;

  auto operator=(const BlockingPool &runtime) -> BlockingPool & = delete;

  auto operator=(BlockingPool &&runtime) -> BlockingPool & = delete;

  ~BlockingPool();

 private:
  std::vector<Worker> workers_;
  std::vector<std::thread> worker_ctx_;
};
}  // namespace blocking

#endif  // XYWEBSERVER_EVENT_RUNTIME_BLOCKING_H_