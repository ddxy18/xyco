#include "driver.h"

#include "io/utils.h"
#include "spdlog/fmt/chrono.h"

auto xyco::time::TimeExtra::print() const -> std::string {
  return fmt::format("TimeExtra{{expire_time_={}}}", expire_time_);
}

xyco::time::TimeExtra::TimeExtra(
    std::chrono::time_point<std::chrono::system_clock> expire_time)
    : expire_time_(expire_time) {}

auto xyco::time::TimeRegistry::Register(runtime::Event &ev)
    -> io::IoResult<void> {
  wheel_.insert_event(ev);

  return io::IoResult<void>::ok();
}

// TODO(dongxiaoyu): support update expire time and cancel event
auto xyco::time::TimeRegistry::reregister(runtime::Event &ev)
    -> io::IoResult<void> {
  return io::IoResult<void>::ok();
}

auto xyco::time::TimeRegistry::deregister(runtime::Event &ev)
    -> io::IoResult<void> {
  return io::IoResult<void>::ok();
}

auto xyco::time::TimeRegistry::select(runtime::Events &events,
                                      std::chrono::milliseconds timeout)
    -> io::IoResult<void> {
  wheel_.expire(events);

  return io::IoResult<void>::ok();
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