#include "registry.h"

#include <sys/epoll.h>
#include <unistd.h>

#include <array>
#include <vector>

#include "extra.h"

auto to_sys(xyco::io::epoll::IoExtra::Interest interest) -> int {
  switch (interest) {
    case xyco::io::epoll::IoExtra::Interest::Read:
      return EPOLLIN | EPOLLRDHUP | EPOLLONESHOT;
    case xyco::io::epoll::IoExtra::Interest::Write:
      return EPOLLOUT | EPOLLRDHUP | EPOLLONESHOT;
    case xyco::io::epoll::IoExtra::Interest::All:
      return EPOLLIN | EPOLLOUT | EPOLLRDHUP | EPOLLONESHOT;
  }
}

auto to_state(uint32_t events) -> xyco::io::epoll::IoExtra::State {
  xyco::io::epoll::IoExtra::State state{};
  state.set_field<xyco::io::epoll::IoExtra::State::Registered>();
  if ((events & EPOLLERR) != 0U || (events & EPOLLHUP) != 0U ||
      (events & EPOLLRDHUP) != 0U || (events & EPOLLPRI) != 0U) {
    state.set_field<xyco::io::epoll::IoExtra::State::Error>();
    return state;
  }
  if ((events & EPOLLIN) != 0U) {
    state.set_field<xyco::io::epoll::IoExtra::State::Readable>();
  }
  if ((events & EPOLLOUT) != 0U) {
    state.set_field<xyco::io::epoll::IoExtra::State::Writable>();
  }
  if (((~events & EPOLLIN) != 0U) && ((~events & EPOLLOUT) != 0U)) {
    state.set_field<xyco::io::epoll::IoExtra::State::Pending>();
  }
  return state;
}

template <>
struct std::formatter<epoll_event> : public std::formatter<bool> {
  template <typename FormatContext>
  auto format(const epoll_event &event, FormatContext &ctx) const
      -> decltype(ctx.out()) {
    return std::format_to(ctx.out(), "epoll_event{{events={:x},data={}}}",
                          event.events,
                          *static_cast<xyco::runtime::Event *>(event.data.ptr));
  }
};

auto xyco::io::epoll::IoRegistry::Register(
    std::shared_ptr<runtime::Event> event) -> utils::Result<void> {
  auto *extra = dynamic_cast<io::epoll::IoExtra *>(event->extra_.get());
  epoll_event epoll_event{
      .events = static_cast<uint32_t>(to_sys((extra->interest_))),
      .data = {.ptr = event.get()}};

  auto result = utils::into_sys_result(
      ::epoll_ctl(epfd_, EPOLL_CTL_ADD, extra->fd_, &epoll_event));
  if (result.is_ok()) {
    TRACE("epoll_ctl add:{}", epoll_event);
    extra->state_.set_field<io::epoll::IoExtra::State::Registered>();
    registered_events_.push_back(event);
  }

  return result;
}

auto xyco::io::epoll::IoRegistry::reregister(
    std::shared_ptr<runtime::Event> event) -> utils::Result<void> {
  auto *extra = dynamic_cast<io::epoll::IoExtra *>(event->extra_.get());
  epoll_event epoll_event{static_cast<uint32_t>(to_sys(extra->interest_))};
  epoll_event.data.ptr = event.get();

  auto result = utils::into_sys_result(
      ::epoll_ctl(epfd_, EPOLL_CTL_MOD, extra->fd_, &epoll_event));
  if (result.is_ok()) {
    TRACE("epoll_ctl mod:{}", epoll_event);
  }

  return result;
}

auto xyco::io::epoll::IoRegistry::deregister(
    std::shared_ptr<runtime::Event> event) -> utils::Result<void> {
  auto *extra = dynamic_cast<io::epoll::IoExtra *>(event->extra_.get());
  epoll_event epoll_event{static_cast<uint32_t>(to_sys(extra->interest_))};
  epoll_event.data.ptr = event.get();

  auto result = utils::into_sys_result(
      ::epoll_ctl(epfd_, EPOLL_CTL_DEL, extra->fd_, &epoll_event));
  if (result.is_ok()) {
    registered_events_.erase(
        std::find(registered_events_.begin(), registered_events_.end(), event));
    extra->state_.set_field<io::epoll::IoExtra::State::Registered, false>();
    TRACE("epoll_ctl del:{}", epoll_event);
  }

  return result;
}

auto xyco::io::epoll::IoRegistry::select(runtime::Events &events,
                                         std::chrono::milliseconds timeout)
    -> utils::Result<void> {
  static auto epoll_events = std::array<epoll_event, MAX_EVENTS>();

  auto select_result = utils::into_sys_result(
      ::epoll_wait(epfd_, epoll_events.data(), MAX_EVENTS,
                   static_cast<int>(std::min(timeout, MAX_TIMEOUT).count())));
  if (select_result.is_err()) {
    auto err = select_result.unwrap_err();
    if (err.errno_ != EINTR) {
      return utils::Result<void>::err(err);
    }
    return utils::Result<void>::ok();
  }
  auto ready_len = select_result.unwrap();
  for (auto i = 0; i < ready_len; i++) {
    auto ready_event = std::find_if(
        registered_events_.begin(), registered_events_.end(),
        [&](auto &registered_event) {
          return epoll_events.at(i).data.ptr == registered_event.get();
        });
    dynamic_cast<io::epoll::IoExtra *>(ready_event->get()->extra_.get())
        ->state_ = to_state(epoll_events.at(i).events);
    TRACE("select {}", *ready_event->get());
    events.push_back(*ready_event);
  }
  return utils::Result<void>::ok();
}

xyco::io::epoll::IoRegistry::IoRegistry(int entries)
    : epfd_(::epoll_create(entries)) {
  if (epfd_ == -1) {
    utils::panic();
  }
}

xyco::io::epoll::IoRegistry::~IoRegistry() { ::close(epfd_); }