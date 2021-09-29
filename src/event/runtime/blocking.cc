#include "blocking.h"

#include <thread>

auto blocking::Task::operator()() -> void { inner_(); }

auto blocking::Worker::run() -> void {
  while (!tasks_.empty()) {
    mutex_.lock();
    auto task = tasks_.front();
    tasks_.pop();
    mutex_.unlock();
    task();
  }
}

auto blocking::BlockingPool::run() -> void {
  for (auto &worker : workers_) {
    std::thread t([&]() {
      while (true) {
        worker.run();
      }
    });
    t.detach();
  }
}

auto blocking::BlockingPool::spawn(blocking::Task task) -> void {
  auto n = 0;
  auto worker = std::min_element(
      workers_.begin(), workers_.end(),
      [](Worker &a, Worker &b) { return a.tasks_.size() < b.tasks_.size(); });
  worker->mutex_.lock();
  worker->tasks_.push(task);
  worker->mutex_.unlock();
}