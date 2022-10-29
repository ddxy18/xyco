#ifndef XYCO_NET_EPOLL_REGISTRY_H_
#define XYCO_NET_EPOLL_REGISTRY_H_

#include <vector>

#include "runtime/registry.h"

namespace xyco::io::epoll {
class IoRegistry : public runtime::Registry {
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

  IoRegistry(int entries);

  IoRegistry(const IoRegistry &epoll) = delete;

  IoRegistry(IoRegistry &&epoll) = delete;

  auto operator=(const IoRegistry &epoll) -> IoRegistry & = delete;

  auto operator=(IoRegistry &&epoll) -> IoRegistry & = delete;

  ~IoRegistry() override;

 private:
  constexpr static std::chrono::milliseconds MAX_TIMEOUT =
      std::chrono::milliseconds(1);
  constexpr static int MAX_EVENTS = 10000;

  int epfd_;
  std::vector<std::shared_ptr<runtime::Event>> registered_events_;
};
}  // namespace xyco::io::epoll

#endif  // XYCO_NET_EPOLL_REGISTRY_H_