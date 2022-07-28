#ifndef XYCO_NET_EPOLL_H_
#define XYCO_NET_EPOLL_H_

#include <vector>

#include "runtime/registry.h"

namespace xyco::net {
class NetRegistry : public runtime::Registry {
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

  NetRegistry();

  NetRegistry(const NetRegistry &epoll) = delete;

  NetRegistry(NetRegistry &&epoll) = delete;

  auto operator=(const NetRegistry &epoll) -> NetRegistry & = delete;

  auto operator=(NetRegistry &&epoll) -> NetRegistry & = delete;

  ~NetRegistry() override;

 private:
  constexpr static std::chrono::milliseconds MAX_TIMEOUT =
      std::chrono::milliseconds(2);
  constexpr static int MAX_EVENTS = 10000;

  int epfd_;
  std::vector<std::shared_ptr<runtime::Event>> registered_events_;
};
}  // namespace xyco::net

#endif  // XYCO_NET_EPOLL_H_