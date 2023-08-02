#include "xyco/time/driver.h"

auto xyco::time::TimeExtra::print() const -> std::string {
  return std::format("{}", *this);
}

xyco::time::TimeExtra::TimeExtra(
    std::chrono::time_point<std::chrono::system_clock> expire_time)
    : expire_time_(expire_time) {}

auto xyco::time::TimeRegistryImpl::Register(
    std::shared_ptr<runtime::Event> event) -> utils::Result<void> {
  // TODO(xiaoyu): Puts the lock into wheel to shorten lock time
  std::scoped_lock<std::mutex> lock_guard(select_mutex_);
  wheel_.insert_event(event);

  return utils::Result<void>::ok();
}

// TODO(dongxiaoyu): support update expire time and cancel event
auto xyco::time::TimeRegistryImpl::reregister(
    std::shared_ptr<runtime::Event> event) -> utils::Result<void> {
  return utils::Result<void>::ok();
}

auto xyco::time::TimeRegistryImpl::deregister(
    std::shared_ptr<runtime::Event> event) -> utils::Result<void> {
  return utils::Result<void>::ok();
}

auto xyco::time::TimeRegistryImpl::select(runtime::Events &events,
                                          std::chrono::milliseconds timeout)
    -> utils::Result<void> {
  std::scoped_lock<std::mutex> lock_guard(select_mutex_);
  wheel_.expire(events);

  return utils::Result<void>::ok();
}
