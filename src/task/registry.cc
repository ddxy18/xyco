module;

#include <algorithm>
#include <format>
#include <functional>
#include <mutex>

module xyco.task;

import xyco.runtime_ctx;

auto xyco::task::BlockingExtra::print() const -> std::string {
  return std::format("{}", *this);
}

xyco::task::BlockingExtra::BlockingExtra(std::function<void()> before_extra)
    : before_extra_(std::move(before_extra)) {}

auto xyco::task::Task::operator()() -> void { inner_(); }

xyco::task::Task::Task(std::function<void()> task) : inner_(std::move(task)) {}

auto xyco::task::BlockingWorker::run(BlockingRegistryImpl& blocking_registry)
    -> void {
  while (!end_) {
    std::unique_lock<std::mutex> lock_guard(mutex_);
    {
      std::scoped_lock<std::mutex> blocking_registry_lock_guard(
          blocking_registry.mutex_);
      while (!tasks_.empty()) {
        auto task = tasks_.front();
        tasks_.pop();
        lock_guard.unlock();
        task();
        lock_guard.lock();
      }
    }
    cv_.wait(lock_guard, [&]() { return !tasks_.empty() || end_; });
  }
}

auto xyco::task::BlockingPool::run(BlockingRegistryImpl& blocking_registry)
    -> void {
  for (auto& worker : workers_) {
    worker_ctx_.emplace_back([&]() { worker.run(blocking_registry); });
  }
}

auto xyco::task::BlockingPool::spawn(task::Task task) -> void {
  auto worker = std::min_element(
      workers_.begin(), workers_.end(), [](auto& worker1, auto& worker2) {
        return worker1.tasks_.size() < worker2.tasks_.size();
      });
  {
    std::scoped_lock<std::mutex> lock_guard(worker->mutex_);
    worker->tasks_.push(std::move(task));
  }
  worker->cv_.notify_one();
}

xyco::task::BlockingPool::BlockingPool(uintptr_t worker_num)
    : workers_(worker_num) {}

xyco::task::BlockingPool::~BlockingPool() {
  for (auto& worker : workers_) {
    worker.end_ = true;
    worker.cv_.notify_one();
  }
  for (auto& thread : worker_ctx_) {
    thread.join();
  }
}

xyco::task::BlockingRegistryImpl::BlockingRegistryImpl(uintptr_t woker_num)
    : pool_(woker_num) {
  pool_.run(*this);
}

auto xyco::task::BlockingRegistryImpl::Register(
    std::shared_ptr<runtime::Event> event) -> utils::Result<void> {
  {
    std::scoped_lock<std::mutex> lock_guard(mutex_);
    events_.push_back(event);
  }
  pool_.spawn(task::Task(
      dynamic_cast<BlockingExtra*>(event->extra_.get())->before_extra_));
  return {};
}

auto xyco::task::BlockingRegistryImpl::reregister(
    [[maybe_unused]] std::shared_ptr<runtime::Event> event)
    -> utils::Result<void> {
  return {};
}

auto xyco::task::BlockingRegistryImpl::deregister(
    [[maybe_unused]] std::shared_ptr<runtime::Event> event)
    -> utils::Result<void> {
  return {};
}

auto xyco::task::BlockingRegistryImpl::select(
    runtime::Events& events,
    [[maybe_unused]] std::chrono::milliseconds timeout) -> utils::Result<void> {
  decltype(events_) new_events;

  std::scoped_lock<std::mutex> lock_guard(mutex_);
  std::copy_if(std::begin(events_), std::end(events_),
               std::back_inserter(new_events), [](auto event) {
                 return dynamic_cast<BlockingExtra*>(event->extra_.get())
                            ->after_extra_ == nullptr;
               });
  std::copy_if(std::begin(events_), std::end(events_),
               std::back_inserter(events), [](auto event) {
                 return dynamic_cast<BlockingExtra*>(event->extra_.get())
                            ->after_extra_ != nullptr;
               });
  events_ = new_events;

  return {};
}
