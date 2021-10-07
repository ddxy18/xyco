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
  explicit Task(const std::function<void()> &task);

  auto operator()() -> void;

 private:
  const std::function<void()> &inner_;
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

  ~BlockingPool();

 private:
  std::vector<Worker> workers_;
  std::vector<std::thread> worker_ctx_;
};
}  // namespace blocking

#endif  // XYWEBSERVER_EVENT_RUNTIME_BLOCKING_H_