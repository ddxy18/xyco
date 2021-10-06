#ifndef XYWEBSERVER_EVENT_RUNTIME_DRIVER_H_
#define XYWEBSERVER_EVENT_RUNTIME_DRIVER_H_

#include "blocking.h"
#include "poll.h"

namespace net {
class Epoll;
}

namespace runtime {
class BlockingPoll : public reactor::Registry {
 public:
  explicit BlockingPoll(int woker_num);

  [[nodiscard]] auto Register(reactor::Event* ev) -> IoResult<void> override;

  [[nodiscard]] auto reregister(reactor::Event* ev) -> IoResult<void> override;

  [[nodiscard]] auto deregister(reactor::Event* ev) -> IoResult<void> override;

  [[nodiscard]] auto select(reactor::Events* events, int timeout)
      -> IoResult<void> override;

 private:
  reactor::Events events_;
  std::mutex mutex_;
  blocking::BlockingPool pool_;
};

class Driver {
 public:
  auto poll() -> void;

  auto net_handle() -> reactor::Poll*;

  auto blocking_handle() -> reactor::Poll*;

  explicit Driver(uintptr_t blocking_num);

 private:
  reactor::Poll net_poll_;
  reactor::Poll blocking_poll_;
};
}  // namespace runtime

#endif  // XYWEBSERVER_EVENT_RUNTIME_DRIVER_H_