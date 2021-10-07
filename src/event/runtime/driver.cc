#include "driver.h"

#include "event/net/epoll.h"

runtime::BlockingPoll::BlockingPoll(int woker_num) : pool_(woker_num) {
  pool_.run();
}

auto runtime::BlockingPoll::Register(reactor::Event* ev) -> IoResult<void> {
  {
    std::scoped_lock<std::mutex> lock_guard(mutex_);
    events_.push_back(ev);
  }
  pool_.spawn(
      blocking::Task(*static_cast<std::function<void()>*>(ev->before_extra_)));
  return IoResult<void>::ok();
}

auto runtime::BlockingPoll::reregister(reactor::Event* ev) -> IoResult<void> {
  return IoResult<void>::ok();
}

auto runtime::BlockingPoll::deregister(reactor::Event* ev) -> IoResult<void> {
  return IoResult<void>::ok();
}

auto runtime::BlockingPoll::select(reactor::Events* events, int timeout)
    -> IoResult<void> {
  auto i = 0;
  decltype(events_) new_events;

  std::scoped_lock<std::mutex> lock_guard(mutex_);
  std::copy_if(std::begin(events_), std::end(events_),
               std::back_inserter(new_events),
               [](auto* ev) { return ev->after_extra_ == nullptr; });
  std::copy_if(std::begin(events_), std::end(events_),
               std::back_inserter(*events),
               [](auto* ev) { return ev->after_extra_ != nullptr; });
  events_ = new_events;

  return IoResult<void>::ok();
}

auto runtime::Driver::poll() -> void {
  reactor::Events events;
  net_poll_.poll(&events, net::Epoll::MAX_TIMEOUT_MS);
  blocking_poll_.poll(&events, net::Epoll::MAX_TIMEOUT_MS);
}

auto runtime::Driver::net_handle() -> reactor::Poll* { return &net_poll_; }

auto runtime::Driver::blocking_handle() -> reactor::Poll* {
  return &blocking_poll_;
}

runtime::Driver::Driver(uintptr_t blocking_num)
    : net_poll_(std::make_unique<net::Epoll>()),
      blocking_poll_(std::make_unique<BlockingPoll>(blocking_num)) {}