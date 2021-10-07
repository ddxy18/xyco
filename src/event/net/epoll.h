#ifndef XYWEBSERVER_EVENT_NET_EPOLL_H_
#define XYWEBSERVER_EVENT_NET_EPOLL_H_

#include "event/runtime/poll.h"

namespace net {
class Epoll : public reactor::Registry {
 public:
  static const int MAX_TIMEOUT_MS = 2;
  static const int MAX_EVENTS = 10000;

  Epoll();

  [[nodiscard]] auto Register(reactor::Event *ev) -> IoResult<void> override;

  [[nodiscard]] auto reregister(reactor::Event *ev) -> IoResult<void> override;

  [[nodiscard]] auto deregister(reactor::Event *ev) -> IoResult<void> override;

  [[nodiscard]] auto select(reactor::Events *events, int timeout)
      -> IoResult<void> override;

 private:
  int epfd_;
};
}  // namespace net

#endif  // XYWEBSERVER_EVENT_NET_EPOLL_H_