#include "driver.h"

#include "runtime.h"

runtime::BlockingRegistry::BlockingRegistry(uintptr_t woker_num)
    : pool_(woker_num) {
  pool_.run();
}

auto runtime::BlockingRegistry::Register(reactor::Event& ev,
                                         reactor::Interest interest)
    -> IoResult<void> {
  {
    std::scoped_lock<std::mutex> lock_guard(mutex_);
    events_.push_back(ev);
  }
  pool_.spawn(blocking::Task(ev.before_extra_));
  return IoResult<void>::ok();
}

auto runtime::BlockingRegistry::reregister(reactor::Event& ev,
                                           reactor::Interest interest)
    -> IoResult<void> {
  return IoResult<void>::ok();
}

auto runtime::BlockingRegistry::deregister(reactor::Event& ev,
                                           reactor::Interest interest)
    -> IoResult<void> {
  return IoResult<void>::ok();
}

auto runtime::BlockingRegistry::select(reactor::Events& events, int timeout)
    -> IoResult<void> {
  auto i = 0;
  decltype(events_) new_events;

  std::scoped_lock<std::mutex> lock_guard(mutex_);
  std::copy_if(std::begin(events_), std::end(events_),
               std::back_inserter(new_events),
               [](reactor::Event& ev) { return ev.after_extra_ == nullptr; });
  std::copy_if(std::begin(events_), std::end(events_),
               std::back_inserter(events),
               [](reactor::Event& ev) { return ev.after_extra_ != nullptr; });
  events_ = new_events;

  return IoResult<void>::ok();
}

auto runtime::Driver::poll() -> void {
  reactor::Events events;
  io_registry_.select(events, net::NetRegistry::MAX_TIMEOUT_MS).unwrap();

  RuntimeCtx::get_ctx()->wake(events);
  blocking_registry_.select(events, net::NetRegistry::MAX_TIMEOUT_MS).unwrap();
  RuntimeCtx::get_ctx()->wake(events);
}

auto runtime::Driver::net_handle() -> io::IoRegistry* { return &io_registry_; }

auto runtime::Driver::blocking_handle() -> BlockingRegistry* {
  return &blocking_registry_;
}

runtime::Driver::Driver(uintptr_t blocking_num)
    : blocking_registry_(blocking_num) {}