module;

#include <chrono>
#include <format>
#include <memory>
#include <mutex>

module xyco.time;

import xyco.runtime_ctx;

auto xyco::time::TimeExtra::print() const -> std::string { return std::format("{}", *this); }

xyco::time::TimeExtra::TimeExtra(std::chrono::time_point<std::chrono::system_clock> expire_time)
    : expire_time_(expire_time) {}

auto xyco::time::TimeRegistryImpl::Register(std::shared_ptr<runtime::Event> event)
    -> utils::Result<void> {
  // TODO(xiaoyu): Puts the lock into wheel to shorten lock time
  std::scoped_lock<std::mutex> lock_guard(select_mutex_);
  wheel_.insert_event(event);

  return {};
}

// TODO(dongxiaoyu): support update expire time and cancel event
auto xyco::time::TimeRegistryImpl::reregister(
    [[maybe_unused]] std::shared_ptr<runtime::Event> event) -> utils::Result<void> {
  return {};
}

auto xyco::time::TimeRegistryImpl::deregister(
    [[maybe_unused]] std::shared_ptr<runtime::Event> event) -> utils::Result<void> {
  return {};
}

auto xyco::time::TimeRegistryImpl::select(runtime::Events &events,
                                          [[maybe_unused]] std::chrono::milliseconds timeout)
    -> utils::Result<void> {
  std::scoped_lock<std::mutex> lock_guard(select_mutex_);
  wheel_.expire(events);

  return {};
}
