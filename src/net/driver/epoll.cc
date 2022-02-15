#include "epoll.h"

#include <sys/epoll.h>
#include <unistd.h>

#include <array>
#include <vector>

#include "io/driver.h"

auto to_sys(xyco::io::IoExtra::Interest interest) -> int {
  switch (interest) {
    case xyco::io::IoExtra::Interest::Read:
      return EPOLLIN | EPOLLET;
    case xyco::io::IoExtra::Interest::Write:
      return EPOLLOUT | EPOLLET;
    case xyco::io::IoExtra::Interest::All:
      return EPOLLIN | EPOLLOUT | EPOLLET;
  }
}

auto to_state(uint32_t events) -> xyco::io::IoExtra::State {
  if ((events & EPOLLIN) != 0U) {
    if ((events & EPOLLOUT) != 0U) {
      return xyco::io::IoExtra::State::All;
    }
  }
  if ((events & EPOLLIN) != 0U) {
    return xyco::io::IoExtra::State::Readable;
  }
  if ((events & EPOLLOUT) != 0U) {
    return xyco::io::IoExtra::State::Writable;
  }
  return xyco::io::IoExtra::State::Pending;
}

template <>
struct fmt::formatter<epoll_event> : public fmt::formatter<bool> {
  template <typename FormatContext>
  auto format(const epoll_event &event, FormatContext &ctx) const
      -> decltype(ctx.out()) {
    return format_to(ctx.out(), "epoll_event{{events={:x},data={}}}",
                     event.events,
                     *static_cast<xyco::runtime::Event *>(event.data.ptr));
  }
};

auto xyco::net::NetRegistry::Register(runtime::Event &event)
    -> io::IoResult<void> {
  auto *extra = dynamic_cast<io::IoExtra *>(event.extra_);
  epoll_event epoll_event{
      .events = static_cast<uint32_t>(to_sys((extra->interest_))),
      .data = {.ptr = &event}};

  auto result = io::into_sys_result(
      epoll_ctl(epfd_, EPOLL_CTL_ADD, extra->fd_, &epoll_event));
  if (result.is_ok()) {
    TRACE("epoll_ctl add:{}", epoll_event);
  }

  return result;
}

auto xyco::net::NetRegistry::reregister(runtime::Event &event)
    -> io::IoResult<void> {
  auto *extra = dynamic_cast<io::IoExtra *>(event.extra_);
  epoll_event epoll_event{static_cast<uint32_t>(to_sys(extra->interest_))};
  epoll_event.data.ptr = &event;

  auto result = io::into_sys_result(
      epoll_ctl(epfd_, EPOLL_CTL_MOD, extra->fd_, &epoll_event));
  if (result.is_ok()) {
    TRACE("epoll_ctl mod:{}", epoll_event);
  }

  return result;
}

auto xyco::net::NetRegistry::deregister(runtime::Event &event)
    -> io::IoResult<void> {
  auto *extra = dynamic_cast<io::IoExtra *>(event.extra_);
  epoll_event epoll_event{static_cast<uint32_t>(to_sys(extra->interest_))};
  epoll_event.data.ptr = &event;

  auto result = io::into_sys_result(
      epoll_ctl(epfd_, EPOLL_CTL_DEL, extra->fd_, &epoll_event));
  if (result.is_ok()) {
    TRACE("epoll_ctl del:{}", epoll_event);
  }

  return result;
}

auto xyco::net::NetRegistry::select(runtime::Events &events,
                                    std::chrono::milliseconds timeout)
    -> io::IoResult<void> {
  auto final_timeout = timeout;
  auto final_max_events = MAX_EVENTS;
  if (timeout > MAX_TIMEOUT) {
    final_timeout = MAX_TIMEOUT;
  }

  static auto epoll_events = std::array<epoll_event, MAX_EVENTS>();

  auto select_result = io::into_sys_result(
      ::epoll_wait(epfd_, epoll_events.data(), final_max_events,
                   static_cast<int>(final_timeout.count())));
  if (select_result.is_err()) {
    auto err = select_result.unwrap_err();
    if (err.errno_ != EINTR) {
      return io::IoResult<void>::err(err);
    }
    return io::IoResult<void>::ok();
  }
  auto ready_len = select_result.unwrap();
  if (ready_len != 0) {
    TRACE("epoll_wait:{}", ready_len);
  }

  for (auto i = 0; i < ready_len; i++) {
    auto &ready_ev =
        *static_cast<runtime::Event *>((epoll_events.at(i).data.ptr));
    dynamic_cast<io::IoExtra *>(ready_ev.extra_)->state_ =
        to_state(epoll_events.at(i).events);
    events.push_back(ready_ev);
  }
  return io::IoResult<void>::ok();
}

xyco::net::NetRegistry::NetRegistry() : epfd_(epoll_create(1)) {
  if (epfd_ == -1) {
    panic();
  }
}

xyco::net::NetRegistry::~NetRegistry() { close(epfd_); }