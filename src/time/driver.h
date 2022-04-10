#ifndef XYCO_TIME_DRIVER_H
#define XYCO_TIME_DRIVER_H

#include "runtime/registry.h"
#include "wheel.h"

namespace xyco::time {
class TimeExtra : public runtime::Extra {
 public:
  [[nodiscard]] auto print() const -> std::string override;

  TimeExtra(std::chrono::time_point<std::chrono::system_clock> expire_time =
                std::chrono::time_point<std::chrono::system_clock>::min());

  std::chrono::time_point<std::chrono::system_clock> expire_time_;
};

class TimeRegistry : public runtime::Registry {
 public:
  [[nodiscard]] auto Register(std::shared_ptr<runtime::Event> event)
      -> io::IoResult<void> override;

  [[nodiscard]] auto reregister(std::shared_ptr<runtime::Event> event)
      -> io::IoResult<void> override;

  [[nodiscard]] auto deregister(std::shared_ptr<runtime::Event> event)
      -> io::IoResult<void> override;

  [[nodiscard]] auto select(runtime::Events &events,
                            std::chrono::milliseconds timeout)
      -> io::IoResult<void> override;

 private:
  Wheel wheel_;
};
}  // namespace xyco::time

template <>
struct fmt::formatter<xyco::time::TimeExtra>
    : public fmt::formatter<std::string> {
  template <typename FormatContext>
  auto format(const xyco::time::TimeExtra &extra, FormatContext &ctx) const
      -> decltype(ctx.out());
};

#endif  // XYCO_TIME_DRIVER_H