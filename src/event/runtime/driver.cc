#include "driver.h"

#include "event/net/epoll.h"

auto runtime::BlockingPoll::Register(reactor::Event* ev) -> IoResult<Void> {
  events_.push_back(ev);
  pool_.spawn(
      blocking::Task(*static_cast<std::function<void()>*>(ev->before_extra_)));
  return Ok<Void, IoError>(Void());
}

auto runtime::BlockingPoll::reregister(reactor::Event* ev) -> IoResult<Void> {
  return Ok<Void, IoError>(Void());
}

auto runtime::BlockingPoll::deregister(reactor::Event* ev) -> IoResult<Void> {
  return Ok<Void, IoError>(Void());
}

auto runtime::BlockingPoll::select(reactor::Events* events, int timeout)
    -> IoResult<Void> {
  auto i = 0;
  decltype(events_) new_events;
  std::copy_if(std::begin(events_), std::end(events_),
               std::back_inserter(new_events),
               [](auto* ev) { return ev->after_extra_ == nullptr; });
  std::copy_if(std::begin(events_), std::end(events_),
               std::back_inserter(*events),
               [](auto* ev) { return ev->after_extra_ != nullptr; });
  events_ = new_events;
  return Ok<Void, IoError>(Void());
}

auto runtime::Driver::poll() -> void {
  reactor::Events events;
  net_poll_.poll(&events, net::Epoll::MAX_TIMEOUT);
  blocking_poll_.poll(&events, net::Epoll::MAX_TIMEOUT);
}

runtime::Driver::Driver(uintptr_t blocking_num)
    : net_poll_(std::make_unique<net::Epoll>()),
      blocking_poll_(std::make_unique<BlockingPoll>(blocking_num)) {}