#ifndef XYCO_IO_IO_URING_REGISTRY_H_
#define XYCO_IO_IO_URING_REGISTRY_H_

#include <liburing.h>

#include <vector>

#include "xyco/runtime/thread_local_registry.h"

namespace xyco::io::uring {
class IoRegistryImpl : public runtime::Registry {
 public:
  constexpr static std::chrono::milliseconds MAX_TIMEOUT =
      std::chrono::milliseconds(1);
  static const int MAX_EVENTS = 10000;

  [[nodiscard]] auto Register(std::shared_ptr<runtime::Event> event)
      -> utils::Result<void> override;

  [[nodiscard]] auto reregister(std::shared_ptr<runtime::Event> event)
      -> utils::Result<void> override;

  [[nodiscard]] auto deregister(std::shared_ptr<runtime::Event> event)
      -> utils::Result<void> override;

  [[nodiscard]] auto select(runtime::Events& events,
                            std::chrono::milliseconds timeout)
      -> utils::Result<void> override;

  IoRegistryImpl(uint32_t entries);

  IoRegistryImpl(const IoRegistryImpl& registry) = delete;

  IoRegistryImpl(IoRegistryImpl&& registry) = delete;

  auto operator=(const IoRegistryImpl& registry) -> IoRegistryImpl& = delete;

  auto operator=(IoRegistryImpl&& registry) -> IoRegistryImpl& = delete;

  ~IoRegistryImpl() override;

 private:
  struct io_uring io_uring_;
  std::vector<std::shared_ptr<runtime::Event>> registered_events_;
};

using IoRegistry = runtime::ThreadLocalRegistry<IoRegistryImpl>;
}  // namespace xyco::io::uring

#endif  // XYCO_IO_IO_URING_REGISTRY_H_
