#ifndef XYCO_NET_EPOLL_REGISTRY_H_
#define XYCO_NET_EPOLL_REGISTRY_H_

#include "xyco/runtime/global_registry.h"

namespace xyco::io::epoll {
class IoRegistryImpl : public runtime::Registry {
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

  IoRegistryImpl(int entries);

  IoRegistryImpl(const IoRegistryImpl &epoll) = delete;

  IoRegistryImpl(IoRegistryImpl &&epoll) = delete;

  auto operator=(const IoRegistryImpl &epoll) -> IoRegistryImpl & = delete;

  auto operator=(IoRegistryImpl &&epoll) -> IoRegistryImpl & = delete;

  ~IoRegistryImpl() override;

 private:
  constexpr static std::chrono::milliseconds MAX_TIMEOUT =
      std::chrono::milliseconds(1);
  constexpr static int MAX_EVENTS = 10000;

  int epfd_;
  std::mutex events_mutex_;
  std::vector<std::shared_ptr<runtime::Event>> registered_events_;

  std::mutex select_mutex_;
};

using IoRegistry = runtime::GlobalRegistry<IoRegistryImpl>;
}  // namespace xyco::io::epoll

#endif  // XYCO_NET_EPOLL_REGISTRY_H_
