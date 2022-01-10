#ifndef XYCO_TIME_DRIVER_H
#define XYCO_TIME_DRIVER_H

#include "runtime/registry.h"
#include "wheel.h"

namespace xyco::time {
class TimeRegistry : public runtime::Registry {
 public:
  [[nodiscard]] auto Register(runtime::Event &ev)
      -> io::IoResult<void> override;

  [[nodiscard]] auto reregister(runtime::Event &ev)
      -> io::IoResult<void> override;

  [[nodiscard]] auto deregister(runtime::Event &ev)
      -> io::IoResult<void> override;

  [[nodiscard]] auto select(runtime::Events &events,
                            std::chrono::milliseconds timeout)
      -> io::IoResult<void> override;

 private:
  Wheel wheel_;
};
}  // namespace xyco::time

#endif  // XYCO_TIME_DRIVER_H