#include "epoll.h"

#include <sys/epoll.h>
#include <unistd.h>

auto to_sys(runtime::Interest interest) -> int {
  switch (interest) {
    case runtime::Interest::Read:
      return EPOLLIN | EPOLLET;
    case runtime::Interest::Write:
      return EPOLLOUT | EPOLLET;
    case runtime::Interest::All:
      return EPOLLIN | EPOLLOUT | EPOLLET;
  }
}

auto to_state(uint32_t events) -> runtime::Event::State {
  if ((events & EPOLLIN) != 0U) {
    if ((events & EPOLLOUT) != 0U) {
      return runtime::Event::State::All;
    }
  }
  if ((events & EPOLLIN) != 0U) {
    return runtime::Event::State::Readable;
  }
  if ((events & EPOLLOUT) != 0U) {
    return runtime::Event::State::Writable;
  }
  return runtime::Event::State::Pending;
}

auto net::NetRegistry::Register(runtime::Event &event,
                                runtime::Interest interest)
    -> io::IoResult<void> {
  epoll_event epoll_event{.events = static_cast<uint32_t>(to_sys(interest)),
                          .data = {.ptr = &event}};

  auto result = io::into_sys_result(
                    epoll_ctl(epfd_, EPOLL_CTL_ADD, event.fd_, &epoll_event))
                    .map([&](auto n) -> void {});
  TRY(result);
  TRACE("register fd:{}\n", event.fd_, epoll_event.data.ptr);

  return result;
}

auto net::NetRegistry::reregister(runtime::Event &event,
                                  runtime::Interest interest)
    -> io::IoResult<void> {
  epoll_event epoll_event{static_cast<uint32_t>(to_sys(interest))};
  epoll_event.data.ptr = &event;

  auto result = io::into_sys_result(
                    epoll_ctl(epfd_, EPOLL_CTL_MOD, event.fd_, &epoll_event))
                    .map([](auto n) {});
  TRY(result);
  TRACE("reregister fd:{}\n", event.fd_, epoll_event.data.ptr);

  return result;
}

auto net::NetRegistry::deregister(runtime::Event &event,
                                  runtime::Interest interest)
    -> io::IoResult<void> {
  epoll_event epoll_event{static_cast<uint32_t>(to_sys(interest))};
  epoll_event.data.ptr = &event;

  auto result = io::into_sys_result(
                    epoll_ctl(epfd_, EPOLL_CTL_DEL, event.fd_, &epoll_event))
                    .map([](auto n) {});
  TRY(result);
  TRACE("deregister fd:{}\n", event.fd_, epoll_event.data.ptr);

  return result;
}

auto net::NetRegistry::select(runtime::Events &events, int timeout)
    -> io::IoResult<void> {
  auto final_timeout = timeout;
  auto final_max_events = MAX_EVENTS;
  if (timeout < 0 || timeout > MAX_TIMEOUT_MS) {
    final_timeout = MAX_TIMEOUT_MS;
  }

  static auto epoll_events = std::array<epoll_event, MAX_EVENTS>();

  auto ready_len =
      epoll_wait(epfd_, epoll_events.data(), final_max_events, final_timeout);
  auto result = io::into_sys_result(ready_len).map([](auto n) {});
  TRY(result);
  TRACE("epoll_wait:{}\n", ready_len);

  for (auto i = 0; i < ready_len; i++) {
    auto &ready_ev =
        *static_cast<runtime::Event *>((epoll_events.at(i).data.ptr));
    ready_ev.state_ = to_state(epoll_events.at(i).events);
    events.push_back(ready_ev);
  }
  return io::IoResult<void>::ok();
}

net::NetRegistry::NetRegistry() : epfd_(epoll_create(1)) {
  if (epfd_ == -1) {
    panic();
  }
}

net::NetRegistry::~NetRegistry() {
  // FIXME(dongxiaoyu): replace global epoll with per worker epoll
  close(epfd_);
}