#include "driver.h"

#include "event/net/epoll.h"
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

[[nodiscard]] auto runtime::IoRegistry::Register(reactor::Event& ev,
                                                 reactor::Interest interest)
    -> IoResult<void> {
  return register_local(ev, interest);
}

[[nodiscard]] auto runtime::IoRegistry::reregister(reactor::Event& ev,
                                                   reactor::Interest interest)
    -> IoResult<void> {
  return reregister_local(ev, interest);
}

[[nodiscard]] auto runtime::IoRegistry::deregister(reactor::Event& ev,
                                                   reactor::Interest interest)
    -> IoResult<void> {
  return deregister_local(ev, interest);
}

[[nodiscard]] auto runtime::IoRegistry::register_local(
    reactor::Event& ev, reactor::Interest interest) -> IoResult<void> {
  auto current_thread =
      RuntimeCtx::get_ctx()->workers_.find(std::this_thread::get_id());
  return current_thread->second->get_epoll_registry().Register(ev, interest);
}

[[nodiscard]] auto runtime::IoRegistry::reregister_local(
    reactor::Event& ev, reactor::Interest interest) -> IoResult<void> {
  auto current_thread =
      RuntimeCtx::get_ctx()->workers_.find(std::this_thread::get_id());
  return current_thread->second->get_epoll_registry().reregister(ev, interest);
}

[[nodiscard]] auto runtime::IoRegistry::deregister_local(
    reactor::Event& ev, reactor::Interest interest) -> IoResult<void> {
  auto current_thread =
      RuntimeCtx::get_ctx()->workers_.find(std::this_thread::get_id());
  return current_thread->second->get_epoll_registry().deregister(ev, interest);
}

[[nodiscard]] auto runtime::IoRegistry::select(reactor::Events& events,
                                               int timeout) -> IoResult<void> {
  return IoResult<void>::ok();
}

auto runtime::Driver::poll() -> void {
  reactor::Events events;
  io_registry_.select(events, net::EpollRegistry::MAX_TIMEOUT_MS).unwrap();

  RuntimeCtx::get_ctx()->wake(events);
  blocking_registry_.select(events, net::EpollRegistry::MAX_TIMEOUT_MS)
      .unwrap();
  RuntimeCtx::get_ctx()->wake(events);
}

auto runtime::Driver::net_handle() -> IoRegistry* { return &io_registry_; }

auto runtime::Driver::blocking_handle() -> BlockingRegistry* {
  return &blocking_registry_;
}

runtime::Driver::Driver(uintptr_t blocking_num)
    : io_registry_(), blocking_registry_(blocking_num) {}
