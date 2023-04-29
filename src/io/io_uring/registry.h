#ifndef XYCO_IO_IO_URING_REGISTRY_H_
#define XYCO_IO_IO_URING_REGISTRY_H_

#include <liburing.h>

#include <vector>

#include "runtime/registry.h"

namespace xyco::io::uring {
class IoRegistry : public runtime::GlobalRegistry {
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

  auto register_local(std::shared_ptr<xyco::runtime::Event> event)
      -> xyco::utils::Result<void> override;

  auto reregister_local(std::shared_ptr<xyco::runtime::Event> event)
      -> xyco::utils::Result<void> override;

  auto deregister_local(std::shared_ptr<xyco::runtime::Event> event)
      -> xyco::utils::Result<void> override;

  auto local_registry_init() -> void override;

  IoRegistry(uint32_t entries);

  IoRegistry(const IoRegistry& registry) = delete;

  IoRegistry(IoRegistry&& registry) = delete;

  auto operator=(const IoRegistry& registry) -> IoRegistry& = delete;

  auto operator=(IoRegistry&& registry) -> IoRegistry& = delete;

  ~IoRegistry() override = default;

 private:
  uint32_t uring_capacity_;
};
}  // namespace xyco::io::uring

#endif  // XYCO_IO_IO_URING_REGISTRY_H_
