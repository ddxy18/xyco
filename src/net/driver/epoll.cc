#include "epoll.h"

#include <sys/epoll.h>
#include <unistd.h>

#include <array>
#include <vector>

auto to_sys(xyco::runtime::IoExtra::Interest interest) -> int {
  switch (interest) {
    case xyco::runtime::IoExtra::Interest::Read:
      return EPOLLIN | EPOLLET;
    case xyco::runtime::IoExtra::Interest::Write:
      return EPOLLOUT | EPOLLET;
    case xyco::runtime::IoExtra::Interest::All:
      return EPOLLIN | EPOLLOUT | EPOLLET;
  }
}

auto to_state(uint32_t events) -> xyco::runtime::IoExtra::State {
  if ((events & EPOLLIN) != 0U) {
    if ((events & EPOLLOUT) != 0U) {
      return xyco::runtime::IoExtra::State::All;
    }
  }
  if ((events & EPOLLIN) != 0U) {
    return xyco::runtime::IoExtra::State::Readable;
  }
  if ((events & EPOLLOUT) != 0U) {
    return xyco::runtime::IoExtra::State::Writable;
  }
  return xyco::runtime::IoExtra::State::Pending;
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
  auto extra = std::get<runtime::IoExtra>(event.extra_);
  epoll_event epoll_event{
      .events = static_cast<uint32_t>(to_sys((extra.interest_))),
      .data = {.ptr = &event}};

  auto result = io::into_sys_result(
      epoll_ctl(epfd_, EPOLL_CTL_ADD, extra.fd_, &epoll_event));
  if (result.is_ok()) {
    TRACE("epoll_ctl add:{}", epoll_event);
  }

  return result;
}

auto xyco::net::NetRegistry::reregister(runtime::Event &event)
    -> io::IoResult<void> {
  auto extra = std::get<runtime::IoExtra>(event.extra_);
  epoll_event epoll_event{static_cast<uint32_t>(to_sys(extra.interest_))};
  epoll_event.data.ptr = &event;

  auto result = io::into_sys_result(
      epoll_ctl(epfd_, EPOLL_CTL_MOD, extra.fd_, &epoll_event));
  if (result.is_ok()) {
    TRACE("epoll_ctl mod:{}", epoll_event);
  }

  return result;
}

auto xyco::net::NetRegistry::deregister(runtime::Event &event)
    -> io::IoResult<void> {
  auto extra = std::get<runtime::IoExtra>(event.extra_);
  epoll_event epoll_event{static_cast<uint32_t>(to_sys(extra.interest_))};
  epoll_event.data.ptr = &event;

  auto result = io::into_sys_result(
      epoll_ctl(epfd_, EPOLL_CTL_DEL, extra.fd_, &epoll_event));
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
  TRACE("epoll_wait:{}", ready_len);

  for (auto i = 0; i < ready_len; i++) {
    auto &ready_ev =
        *static_cast<runtime::Event *>((epoll_events.at(i).data.ptr));
    std::get<runtime::IoExtra>(ready_ev.extra_).state_ =
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

xyco::net::NetRegistry::~NetRegistry() {
  // FIXME(dongxiaoyu): replace global epoll with per worker epoll
  close(epfd_);
}