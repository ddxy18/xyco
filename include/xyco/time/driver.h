#ifndef XYCO_TIME_DRIVER_H
#define XYCO_TIME_DRIVER_H

#include <iomanip>

#include "xyco/runtime/global_registry.h"
#include "xyco/time/wheel.h"

namespace xyco::time {
class TimeExtra : public runtime::Extra {
 public:
  [[nodiscard]] auto print() const -> std::string override;

  TimeExtra(std::chrono::time_point<std::chrono::system_clock> expire_time =
                std::chrono::time_point<std::chrono::system_clock>::min());

  std::chrono::time_point<std::chrono::system_clock> expire_time_;
};

class TimeRegistryImpl : public runtime::Registry {
 public:
  [[nodiscard]] auto Register(std::shared_ptr<runtime::Event> event)
      -> utils::Result<void> override;

  [[nodiscard]] auto reregister(std::shared_ptr<runtime::Event> event)
      -> utils::Result<void> override;

  [[nodiscard]] auto deregister(std::shared_ptr<runtime::Event> event)
      -> utils::Result<void> override;

  [[nodiscard]] auto select(runtime::Events &events,
                            std::chrono::milliseconds timeout)
      -> utils::Result<void> override;

 private:
  Wheel wheel_;

  std::mutex select_mutex_;
};

using TimeRegistry = runtime::GlobalRegistry<TimeRegistryImpl>;
}  // namespace xyco::time

template <>
struct std::formatter<xyco::time::TimeExtra>
    : public std::formatter<std::string> {
  template <typename FormatContext>
  auto format(const xyco::time::TimeExtra &extra, FormatContext &ctx) const
      -> decltype(ctx.out()) {
    // FIXME(xiaoyu): Replaced with `std::format` after libc++ implementation of
    // `std::formatter<std::chrono::sys_time>` is available.
    auto time_t_time = std::chrono::system_clock::to_time_t(extra.expire_time_);
    // NOLINTNEXTLINE(concurrency-mt-unsafe)
    auto *tm_time = std::localtime(&time_t_time);
    std::stringstream stream;
    stream << std::put_time(tm_time, "%F %T");
    return std::format_to(ctx.out(), "TimeExtra{{expire_time_={}}}",
                          stream.str());
  }
};

#endif  // XYCO_TIME_DRIVER_H
