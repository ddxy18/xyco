#include "blocking.h"

#include <thread>

blocking::Task::Task(const std::function<void()> &task) : inner_(task) {}

blocking::BlockingPool::BlockingPool(int worker_num) : workers_(worker_num) {}

auto blocking::Task::operator()() -> void { inner_(); }

auto blocking::Worker::run() -> void {
  while (true) {
    std::unique_lock<std::mutex> lock_guard(mutex_);
    while (!tasks_.empty()) {
      auto task = tasks_.front();
      tasks_.pop();
      lock_guard.unlock();
      task();
      lock_guard.lock();
    }
    cv_.wait(lock_guard, [&]() { return !tasks_.empty(); });
  }
}

auto blocking::BlockingPool::run() -> void {
  for (auto &worker : workers_) {
    std::thread t([&]() { worker.run(); });
    t.detach();
  }
}

auto blocking::BlockingPool::spawn(blocking::Task task) -> void {
  auto n = 0;
  auto worker = std::min_element(
      workers_.begin(), workers_.end(),
      [](Worker &a, Worker &b) { return a.tasks_.size() < b.tasks_.size(); });
  {
    std::scoped_lock<std::mutex> lock_guard(worker->mutex_);
    worker->tasks_.push(task);
  }
  worker->cv_.notify_one();
}