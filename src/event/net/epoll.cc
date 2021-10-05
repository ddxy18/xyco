#include "epoll.h"

#include <sys/epoll.h>

#include <gsl/gsl>
#include <vector>

auto to_sys(reactor::Interest interest) -> int {
  switch (interest) {
    case reactor::Interest::Read:
      return EPOLLIN | EPOLLET;
    case reactor::Interest::Write:
      return EPOLLOUT | EPOLLET;
    case reactor::Interest::All:
      return EPOLLIN | EPOLLOUT | EPOLLET;
  }
}

net::Epoll::Epoll() : epfd_(epoll_create(1)) {
  if (epfd_ == -1) {
    panic();
  }
}

auto net::Epoll::Register(reactor::Event *event) -> IoResult<void> {
  epoll_event epoll_event{static_cast<uint32_t>(to_sys(event->interest_))};
  epoll_event.data.ptr = event;

  auto result =
      into_sys_result(epoll_ctl(epfd_, EPOLL_CTL_ADD, event->fd_, &epoll_event))
          .map([&](auto n) -> void {});
  TRY(result);
  TRACE("register fd:{}\n", event->fd_, epoll_event.data.ptr);

  return result;
}

auto net::Epoll::reregister(reactor::Event *event) -> IoResult<void> {
  epoll_event epoll_event{static_cast<uint32_t>(to_sys(event->interest_))};
  epoll_event.data.ptr = event;

  auto result =
      into_sys_result(epoll_ctl(epfd_, EPOLL_CTL_MOD, event->fd_, &epoll_event))
          .map([](auto n) {});
  TRY(result);
  TRACE("reregister fd:{}\n", event->fd_, epoll_event.data.ptr);

  return result;
}

auto net::Epoll::deregister(reactor::Event *event) -> IoResult<void> {
  epoll_event epoll_event{static_cast<uint32_t>(to_sys(event->interest_))};
  epoll_event.data.ptr = event;

  auto result =
      into_sys_result(epoll_ctl(epfd_, EPOLL_CTL_DEL, event->fd_, &epoll_event))
          .map([](auto n) {});
  TRY(result);
  TRACE("deregister fd:{}\n", event->fd_, epoll_event.data.ptr);

  return result;
}

auto net::Epoll::select(reactor::Events *events, int timeout)
    -> IoResult<void> {
  auto final_timeout = timeout;
  auto final_max_events = MAX_EVENTS;
  if (timeout < 0 || timeout > MAX_TIMEOUT) {
    final_timeout = MAX_TIMEOUT;
  }

  static auto epoll_events = std::array<epoll_event, MAX_EVENTS>();

  auto ready_len =
      epoll_wait(epfd_, epoll_events.data(), final_max_events, final_timeout);
  auto result = into_sys_result(ready_len).map([](auto n) {});
  TRY(result);
  TRACE("epoll_wait:{}\n", ready_len);

  while (ready_len-- > 0) {
    auto *ready_ev =
        gsl::owner<reactor::Event *>((epoll_events.at(ready_len).data.ptr));
    events->push_back(ready_ev);
  }
  return ok<IoError>();
}