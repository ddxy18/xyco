#ifndef XYWEBSERVER_EVENT_NET_EPOLL_H_
#define XYWEBSERVER_EVENT_NET_EPOLL_H_

#include "runtime/registry.h"

namespace net {
class NetRegistry : public reactor::Registry {
 public:
  static const int MAX_TIMEOUT_MS = 2;
  static const int MAX_EVENTS = 10000;

  [[nodiscard]] auto Register(reactor::Event &ev, reactor::Interest interest)
      -> IoResult<void> override;

  [[nodiscard]] auto reregister(reactor::Event &ev, reactor::Interest interest)
      -> IoResult<void> override;

  [[nodiscard]] auto deregister(reactor::Event &ev, reactor::Interest interest)
      -> IoResult<void> override;

  [[nodiscard]] auto select(reactor::Events &events, int timeout)
      -> IoResult<void> override;

  NetRegistry();

  NetRegistry(const NetRegistry &epoll) = delete;

  NetRegistry(NetRegistry &&epoll) = delete;

  auto operator=(const NetRegistry &epoll) -> NetRegistry & = delete;

  auto operator=(NetRegistry &&epoll) -> NetRegistry & = delete;

  ~NetRegistry() override;

 private:
  int epfd_;
};
}  // namespace net

#endif  // XYWEBSERVER_EVENT_NET_EPOLL_H_