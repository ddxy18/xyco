#ifndef XYWEBSERVER_EVENT_RUNTIME_BLOCKING_H_
#define XYWEBSERVER_EVENT_RUNTIME_BLOCKING_H_

#include <functional>
#include <mutex>
#include <queue>
#include <utility>
#include <vector>

namespace blocking {
class Task {
 public:
  explicit Task(const std::function<void()> &task) : inner_(task) {}

  auto operator()() -> void;

 private:
  const std::function<void()> &inner_;
};

class Worker {
  friend class BlockingPool;

 public:
  Worker() : tasks_(), mutex_() {}

 private:
  auto run() -> void;

  std::queue<Task> tasks_;
  std::mutex mutex_;
};

class BlockingPool {
 public:
  explicit BlockingPool(int worker_num) : workers_(worker_num) {}
  auto run() -> void;
  auto spawn(Task task) -> void;

 private:
  std::vector<Worker> workers_;
};
}  // namespace blocking

#endif  // XYWEBSERVER_EVENT_RUNTIME_BLOCKING_H_