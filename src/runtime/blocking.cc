#include "blocking.h"

#include <thread>

auto xyco::runtime::AsyncFutureExtra::print() const -> std::string {
  return fmt::format("AsyncFutureExtra{{}}");
}

xyco::runtime::AsyncFutureExtra::AsyncFutureExtra(
    std::function<void()> before_extra)
    : before_extra_(std::move(before_extra)) {}

auto xyco::runtime::Task::operator()() -> void { inner_(); }

xyco::runtime::Task::Task(std::function<void()> task)
    : inner_(std::move(task)) {}

auto xyco::runtime::BlockingWorker::run(BlockingRegistry& blocking_registry)
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

auto xyco::runtime::BlockingPool::run(BlockingRegistry& blocking_registry)
    -> void {
  for (auto& worker : workers_) {
    worker_ctx_.emplace_back([&]() { worker.run(blocking_registry); });
  }
}

auto xyco::runtime::BlockingPool::spawn(runtime::Task task) -> void {
  auto n = 0;
  auto worker = std::min_element(
      workers_.begin(), workers_.end(),
      [](auto& a, auto& b) { return a.tasks_.size() < b.tasks_.size(); });
  {
    std::scoped_lock<std::mutex> lock_guard(worker->mutex_);
    worker->tasks_.push(std::move(task));
  }
  worker->cv_.notify_one();
}

xyco::runtime::BlockingPool::BlockingPool(uintptr_t worker_num)
    : workers_(worker_num) {}

xyco::runtime::BlockingPool::~BlockingPool() {
  for (auto& worker : workers_) {
    worker.end_ = true;
    worker.cv_.notify_one();
  }
  for (auto& t : worker_ctx_) {
    t.join();
  }
}

xyco::runtime::BlockingRegistry::BlockingRegistry(uintptr_t woker_num)
    : pool_(woker_num) {
  pool_.run(*this);
}

auto xyco::runtime::BlockingRegistry::Register(std::shared_ptr<Event> event)
    -> io::IoResult<void> {
  {
    std::scoped_lock<std::mutex> lock_guard(mutex_);
    events_.push_back(event);
  }
  pool_.spawn(runtime::Task(
      dynamic_cast<AsyncFutureExtra*>(event->extra_.get())->before_extra_));
  return io::IoResult<void>::ok();
}

auto xyco::runtime::BlockingRegistry::reregister(std::shared_ptr<Event> event)
    -> io::IoResult<void> {
  return io::IoResult<void>::ok();
}

auto xyco::runtime::BlockingRegistry::deregister(std::shared_ptr<Event> event)
    -> io::IoResult<void> {
  return io::IoResult<void>::ok();
}

auto xyco::runtime::BlockingRegistry::select(runtime::Events& events,
                                             std::chrono::milliseconds timeout)
    -> io::IoResult<void> {
  auto i = 0;
  decltype(events_) new_events;

  std::scoped_lock<std::mutex> lock_guard(mutex_);
  std::copy_if(
      std::begin(events_), std::end(events_), std::back_inserter(new_events),
      [](auto event) {
        return dynamic_cast<AsyncFutureExtra*>(event.lock()->extra_.get())
                   ->after_extra_ == nullptr;
      });
  std::copy_if(
      std::begin(events_), std::end(events_), std::back_inserter(events),
      [](auto event) {
        return dynamic_cast<AsyncFutureExtra*>(event.lock()->extra_.get())
                   ->after_extra_ != nullptr;
      });
  events_ = new_events;

  return io::IoResult<void>::ok();
}

template <typename FormatContext>
auto fmt::formatter<xyco::runtime::AsyncFutureExtra>::format(
    const xyco::runtime::AsyncFutureExtra& extra, FormatContext& ctx) const
    -> decltype(ctx.out()) {
  return fmt::formatter<std::string>::format(extra.print(), ctx);
}

template auto fmt::formatter<xyco::runtime::AsyncFutureExtra>::format(
    const xyco::runtime::AsyncFutureExtra& extra,
    fmt::basic_format_context<fmt::appender, char>& ctx) const
    -> decltype(ctx.out());