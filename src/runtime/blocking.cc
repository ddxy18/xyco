#include "blocking.h"

auto xyco::runtime::Task::operator()() -> void { inner_(); }

xyco::runtime::Task::Task(std::function<void()> task)
    : inner_(std::move(task)) {}

auto xyco::runtime::BlockingWorker::run() -> void {
  while (!end_) {
    std::unique_lock<std::mutex> lock_guard(mutex_);
    while (!tasks_.empty()) {
      auto task = tasks_.front();
      tasks_.pop();
      lock_guard.unlock();
      task();
      lock_guard.lock();
    }
    cv_.wait(lock_guard, [&]() { return !tasks_.empty() || end_; });
  }
}

auto xyco::runtime::BlockingPool::run() -> void {
  for (auto &worker : workers_) {
    worker_ctx_.emplace_back([&]() { worker.run(); });
  }
}

auto xyco::runtime::BlockingPool::spawn(runtime::Task task) -> void {
  auto n = 0;
  auto worker = std::min_element(
      workers_.begin(), workers_.end(),
      [](auto &a, auto &b) { return a.tasks_.size() < b.tasks_.size(); });
  {
    std::scoped_lock<std::mutex> lock_guard(worker->mutex_);
    worker->tasks_.push(std::move(task));
  }
  worker->cv_.notify_one();
}

xyco::runtime::BlockingPool::BlockingPool(uintptr_t worker_num)
    : workers_(worker_num) {}

xyco::runtime::BlockingPool::~BlockingPool() {
  for (auto &worker : workers_) {
    worker.end_ = true;
    worker.cv_.notify_one();
  }
  for (auto &t : worker_ctx_) {
    t.join();
  }
}