#ifndef XYCO_NET_EPOLL_H_
#define XYCO_NET_EPOLL_H_

#include "runtime/registry.h"

namespace xyco::net {
class NetRegistry : public runtime::Registry {
 public:
  static const int MAX_TIMEOUT_MS = 2;
  static const int MAX_EVENTS = 10000;

  [[nodiscard]] auto Register(runtime::Event &ev, runtime::Interest interest)
      -> io::IoResult<void> override;

  [[nodiscard]] auto reregister(runtime::Event &ev, runtime::Interest interest)
      -> io::IoResult<void> override;

  [[nodiscard]] auto deregister(runtime::Event &ev, runtime::Interest interest)
      -> io::IoResult<void> override;

  [[nodiscard]] auto select(runtime::Events &events, int timeout)
      -> io::IoResult<void> override;

  NetRegistry();

  NetRegistry(const NetRegistry &epoll) = delete;

  NetRegistry(NetRegistry &&epoll) = delete;

  auto operator=(const NetRegistry &epoll) -> NetRegistry & = delete;

  auto operator=(NetRegistry &&epoll) -> NetRegistry & = delete;

  ~NetRegistry() override;

 private:
  int epfd_;
};
}  // namespace xyco::net

#endif  // XYCO_NET_EPOLL_H_