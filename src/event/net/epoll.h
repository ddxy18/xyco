#ifndef XYWEBSERVER_EVENT_NET_EPOLL_H_
#define XYWEBSERVER_EVENT_NET_EPOLL_H_

#include "event/runtime/registry.h"

namespace net {
class EpollRegistry : public reactor::Registry {
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

  EpollRegistry();

  EpollRegistry(const EpollRegistry &epoll) = delete;

  EpollRegistry(EpollRegistry &&epoll) = delete;

  auto operator=(const EpollRegistry &epoll) -> EpollRegistry & = delete;

  auto operator=(EpollRegistry &&epoll) -> EpollRegistry & = delete;

  ~EpollRegistry() override;

 private:
  int epfd_;
};
}  // namespace net

#endif  // XYWEBSERVER_EVENT_NET_EPOLL_H_