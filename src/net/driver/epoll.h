#ifndef XYCO_NET_EPOLL_H_
#define XYCO_NET_EPOLL_H_

#include "runtime/registry.h"

namespace xyco::net {
class NetRegistry : public runtime::Registry {
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
};
}  // namespace xyco::net

#endif  // XYCO_NET_EPOLL_H_