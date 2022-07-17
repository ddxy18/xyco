#ifndef XYCO_IO_DRIVER_IO_URING_H_
#define XYCO_IO_DRIVER_IO_URING_H_

#include <liburing.h>

#include <mutex>
#include <vector>

#include "runtime/registry.h"

namespace xyco::io {
class IoUringRegistry : public runtime::Registry {
 public:
  constexpr static std::chrono::milliseconds MAX_TIMEOUT =
      std::chrono::milliseconds(2);
  static const int MAX_EVENTS = 10000;

  [[nodiscard]] auto Register(std::shared_ptr<runtime::Event> event)
      -> IoResult<void> override;

  [[nodiscard]] auto reregister(std::shared_ptr<runtime::Event> event)
      -> IoResult<void> override;

  [[nodiscard]] auto deregister(std::shared_ptr<runtime::Event> event)
      -> IoResult<void> override;

  [[nodiscard]] auto select(runtime::Events& events,
                            std::chrono::milliseconds timeout)
      -> IoResult<void> override;

  IoUringRegistry(uint32_t entries);

  IoUringRegistry(const IoUringRegistry& registry) = delete;

  IoUringRegistry(IoUringRegistry&& registry) = delete;

  auto operator=(const IoUringRegistry& registry) -> IoUringRegistry& = delete;

  auto operator=(IoUringRegistry&& registry) -> IoUringRegistry& = delete;

  ~IoUringRegistry() override;

 private:
  struct io_uring io_uring_;
  std::vector<std::shared_ptr<runtime::Event>> registered_events_;
};
}  // namespace xyco::io

#endif  // XYCO_IO_DRIVER_IO_URING_H_