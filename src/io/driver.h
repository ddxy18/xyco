#ifndef XYCO_IO_DRIVER_H_
#define XYCO_IO_DRIVER_H_

#include "runtime/registry.h"

namespace xyco::io {
class IoRegistry : public runtime::GlobalRegistry {
 public:
  [[nodiscard]] auto Register(runtime::Event& ev) -> IoResult<void> override;

  [[nodiscard]] auto reregister(runtime::Event& ev) -> IoResult<void> override;

  [[nodiscard]] auto deregister(runtime::Event& ev) -> IoResult<void> override;

  [[nodiscard]] auto register_local(runtime::Event& ev)
      -> IoResult<void> override;

  [[nodiscard]] auto reregister_local(runtime::Event& ev)
      -> IoResult<void> override;

  [[nodiscard]] auto deregister_local(runtime::Event& ev)
      -> IoResult<void> override;

  [[nodiscard]] auto select(runtime::Events& events,
                            std::chrono::milliseconds timeout)
      -> IoResult<void> override;
};
}  // namespace xyco::io
#endif  // XYCO_IO_DRIVER_H_