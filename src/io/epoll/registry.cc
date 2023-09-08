#include "xyco/io/epoll/registry.h"

#include <sys/epoll.h>
#include <unistd.h>

#include <array>
#include <vector>

#include "xyco/io/epoll/extra.h"
#include "xyco/utils/logger.h"
#include "xyco/utils/panic.h"

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

auto xyco::io::epoll::IoRegistryImpl::Register(
    std::shared_ptr<runtime::Event> event) -> utils::Result<void> {
  auto *extra = dynamic_cast<io::epoll::IoExtra *>(event->extra_.get());
  epoll_event epoll_event{
      .events = static_cast<uint32_t>(to_sys((extra->interest_))),
      .data = {.ptr = event.get()}};

  {
    std::scoped_lock<std::mutex> lock_guard(events_mutex_);
    registered_events_.push_back(event);
    extra->state_.set_field<io::epoll::IoExtra::State::Registered>();
  }
  auto result = utils::into_sys_result(
      ::epoll_ctl(epfd_, EPOLL_CTL_ADD, extra->fd_, &epoll_event));
  if (result) {
    TRACE("epoll_ctl add:{}", epoll_event);
  } else {
    std::scoped_lock<std::mutex> lock_guard(events_mutex_);
    registered_events_.erase(
        std::find(registered_events_.begin(), registered_events_.end(), event));
  }

  return result.transform([]([[maybe_unused]] auto result) {});
}

auto xyco::io::epoll::IoRegistryImpl::reregister(
    std::shared_ptr<runtime::Event> event) -> utils::Result<void> {
  auto *extra = dynamic_cast<io::epoll::IoExtra *>(event->extra_.get());
  epoll_event epoll_event{static_cast<uint32_t>(to_sys(extra->interest_)),
                          {.ptr = event.get()}};

  {
    std::scoped_lock<std::mutex> lock_guard(events_mutex_);
    registered_events_.push_back(event);
  }
  auto result = utils::into_sys_result(
      ::epoll_ctl(epfd_, EPOLL_CTL_MOD, extra->fd_, &epoll_event));
  if (result) {
    TRACE("epoll_ctl mod:{}", epoll_event);
  } else {
    std::scoped_lock<std::mutex> lock_guard(events_mutex_);
    registered_events_.erase(
        std::find(registered_events_.begin(), registered_events_.end(), event));
  }

  return result.transform([]([[maybe_unused]] auto result) {});
}

auto xyco::io::epoll::IoRegistryImpl::deregister(
    std::shared_ptr<runtime::Event> event) -> utils::Result<void> {
  auto *extra = dynamic_cast<io::epoll::IoExtra *>(event->extra_.get());
  epoll_event epoll_event{static_cast<uint32_t>(to_sys(extra->interest_)),
                          {.ptr = event.get()}};

  auto result = utils::into_sys_result(
      ::epoll_ctl(epfd_, EPOLL_CTL_DEL, extra->fd_, &epoll_event));
  if (result) {
    TRACE("epoll_ctl del:{}", epoll_event);
  }

  {
    std::scoped_lock<std::mutex> lock_guard(events_mutex_);
    auto event_it =
        std::find(registered_events_.begin(), registered_events_.end(), event);
    if (event_it != registered_events_.end()) {
      registered_events_.erase(event_it);
    }
    extra->state_.set_field<io::epoll::IoExtra::State::Registered, false>();
  }

  return result.transform([]([[maybe_unused]] auto result) {});
}

auto xyco::io::epoll::IoRegistryImpl::select(runtime::Events &events,
                                             std::chrono::milliseconds timeout)
    -> utils::Result<void> {
  static auto epoll_events = std::array<epoll_event, MAX_EVENTS>();

  std::scoped_lock<std::mutex> select_lock_guard(select_mutex_);

  auto select_result = utils::into_sys_result(
      ::epoll_wait(epfd_, epoll_events.data(), MAX_EVENTS,
                   static_cast<int>(std::min(timeout, MAX_TIMEOUT).count())));
  if (!select_result) {
    auto err = select_result.error();
    if (err.errno_ != EINTR) {
      return std::unexpected(err);
    }
    return {};
  }
  auto ready_len = *select_result;
  for (auto i = 0; i < ready_len; i++) {
    std::scoped_lock<std::mutex> lock_guard(events_mutex_);
    auto ready_event = std::find_if(
        registered_events_.begin(), registered_events_.end(),
        [&](auto &registered_event) {
          return epoll_events.at(i).data.ptr == registered_event.get();
        });
    dynamic_cast<io::epoll::IoExtra *>(ready_event->get()->extra_.get())
        ->state_ = to_state(epoll_events.at(i).events);
    TRACE("select {}", **ready_event);
    events.push_back(*ready_event);
    registered_events_.erase(ready_event);
  }
  return {};
}

xyco::io::epoll::IoRegistryImpl::IoRegistryImpl(int entries)
    : epfd_(::epoll_create(entries)) {
  if (epfd_ == -1) {
    utils::panic();
  }
}

xyco::io::epoll::IoRegistryImpl::~IoRegistryImpl() { ::close(epfd_); }
