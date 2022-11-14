#include "driver.h"

#include "spdlog/fmt/chrono.h"

auto xyco::time::TimeExtra::print() const -> std::string {
  return fmt::format("TimeExtra{{expire_time_={}}}", expire_time_);  // NOLINT
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

template <typename FormatContext>
auto fmt::formatter<xyco::time::TimeExtra>::format(
    const xyco::time::TimeExtra &extra, FormatContext &ctx) const
    -> decltype(ctx.out()) {
  return fmt::formatter<std::string>::format(extra.print(), ctx);
}

template auto fmt::formatter<xyco::time::TimeExtra>::format(
    const xyco::time::TimeExtra &extra,
    fmt::basic_format_context<fmt::appender, char> &ctx) const
    -> decltype(ctx.out());