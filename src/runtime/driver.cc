#include "driver.h"

#include "runtime.h"

xyco::runtime::BlockingRegistry::BlockingRegistry(uintptr_t woker_num)
    : pool_(woker_num) {
  pool_.run();
}

auto xyco::runtime::BlockingRegistry::Register(runtime::Event& ev,
                                               runtime::Interest interest)
    -> io::IoResult<void> {
  {
    std::scoped_lock<std::mutex> lock_guard(mutex_);
    events_.push_back(ev);
  }
  pool_.spawn(
      runtime::Task(std::get<AsyncFutureExtra>(ev.extra_).before_extra_));
  return io::IoResult<void>::ok();
}

auto xyco::runtime::BlockingRegistry::reregister(runtime::Event& ev,
                                                 runtime::Interest interest)
    -> io::IoResult<void> {
  return io::IoResult<void>::ok();
}

auto xyco::runtime::BlockingRegistry::deregister(runtime::Event& ev,
                                                 runtime::Interest interest)
    -> io::IoResult<void> {
  return io::IoResult<void>::ok();
}

auto xyco::runtime::BlockingRegistry::select(runtime::Events& events,
                                             int timeout)
    -> io::IoResult<void> {
  auto i = 0;
  decltype(events_) new_events;

  std::scoped_lock<std::mutex> lock_guard(mutex_);
  std::copy_if(std::begin(events_), std::end(events_),
               std::back_inserter(new_events), [](runtime::Event& ev) {
                 return std::get<AsyncFutureExtra>(ev.extra_).after_extra_ ==
                        nullptr;
               });
  std::copy_if(std::begin(events_), std::end(events_),
               std::back_inserter(events), [](runtime::Event& ev) {
                 return std::get<AsyncFutureExtra>(ev.extra_).after_extra_ !=
                        nullptr;
               });
  events_ = new_events;

  return io::IoResult<void>::ok();
}

auto xyco::runtime::Driver::poll() -> void {
  runtime::Events events;
  io_registry_.select(events, net::NetRegistry::MAX_TIMEOUT_MS).unwrap();

  RuntimeCtx::get_ctx()->wake(events);
  blocking_registry_.select(events, net::NetRegistry::MAX_TIMEOUT_MS).unwrap();
  RuntimeCtx::get_ctx()->wake(events);
}

auto xyco::runtime::Driver::net_handle() -> io::IoRegistry* {
  return &io_registry_;
}

auto xyco::runtime::Driver::blocking_handle() -> BlockingRegistry* {
  return &blocking_registry_;
}

xyco::runtime::Driver::Driver(uintptr_t blocking_num)
    : blocking_registry_(blocking_num) {}