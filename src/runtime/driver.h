#ifndef XYWEBSERVER_EVENT_RUNTIME_DRIVER_H_
#define XYWEBSERVER_EVENT_RUNTIME_DRIVER_H_

#include "blocking.h"
#include "io/driver.h"
#include "registry.h"

namespace runtime {
class BlockingRegistry : public reactor::Registry {
 public:
  explicit BlockingRegistry(uintptr_t woker_num);

  [[nodiscard]] auto Register(reactor::Event& ev, reactor::Interest interest)
      -> IoResult<void> override;

  [[nodiscard]] auto reregister(reactor::Event& ev, reactor::Interest interest)
      -> IoResult<void> override;

  [[nodiscard]] auto deregister(reactor::Event& ev, reactor::Interest interest)
      -> IoResult<void> override;

  [[nodiscard]] auto select(reactor::Events& events, int timeout)
      -> IoResult<void> override;

 private:
  reactor::Events events_;
  std::mutex mutex_;
  blocking::BlockingPool pool_;
};

class Driver {
 public:
  auto poll() -> void;

  auto net_handle() -> io::IoRegistry*;

  auto blocking_handle() -> BlockingRegistry*;

  explicit Driver(uintptr_t blocking_num);

 private:
  io::IoRegistry io_registry_;
  BlockingRegistry blocking_registry_;
};
}  // namespace runtime

#endif  // XYWEBSERVER_EVENT_RUNTIME_DRIVER_H_