#ifndef XYWEBSERVER_IO_DRIVER_H_
#define XYWEBSERVER_IO_DRIVER_H_

#include "runtime/registry.h"

namespace io {
class IoRegistry : public reactor::GlobalRegistry {
 public:
  [[nodiscard]] auto Register(reactor::Event& ev, reactor::Interest interest)
      -> IoResult<void> override;

  [[nodiscard]] auto reregister(reactor::Event& ev, reactor::Interest interest)
      -> IoResult<void> override;

  [[nodiscard]] auto deregister(reactor::Event& ev, reactor::Interest interest)
      -> IoResult<void> override;

  [[nodiscard]] auto register_local(reactor::Event& ev,
                                    reactor::Interest interest)
      -> IoResult<void> override;

  [[nodiscard]] auto reregister_local(reactor::Event& ev,
                                      reactor::Interest interest)
      -> IoResult<void> override;

  [[nodiscard]] auto deregister_local(reactor::Event& ev,
                                      reactor::Interest interest)
      -> IoResult<void> override;

  [[nodiscard]] auto select(reactor::Events& events, int timeout)
      -> IoResult<void> override;
};
}  // namespace io
#endif  // XYWEBSERVER_IO_DRIVER_H_