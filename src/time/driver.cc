#include "driver.h"

auto xyco::time::TimeExtra::print() const -> std::string {
  return std::format("{}", *this);
}

xyco::time::TimeExtra::TimeExtra(
    std::chrono::time_point<std::chrono::system_clock> expire_time)
    : expire_time_(expire_time) {}

auto xyco::time::TimeRegistry::Register(std::shared_ptr<runtime::Event> event)
    -> utils::Result<void> {
  wheel_.insert_event(event);

  return utils::Result<void>::ok();
}

// TODO(dongxiaoyu): support update expire time and cancel event
auto xyco::time::TimeRegistry::reregister(std::shared_ptr<runtime::Event> event)
    -> utils::Result<void> {
  return utils::Result<void>::ok();
}

auto xyco::time::TimeRegistry::deregister(std::shared_ptr<runtime::Event> event)
    -> utils::Result<void> {
  return utils::Result<void>::ok();
}

auto xyco::time::TimeRegistry::select(runtime::Events &events,
                                      std::chrono::milliseconds timeout)
    -> utils::Result<void> {
  wheel_.expire(events);

  return utils::Result<void>::ok();
}
