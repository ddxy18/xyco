#ifndef XYCO_RUNTIME_DRIVER_H_
#define XYCO_RUNTIME_DRIVER_H_

#include "blocking.h"
#include "io/driver.h"
#include "registry.h"

namespace runtime {
class BlockingRegistry : public runtime::Registry {
 public:
  explicit BlockingRegistry(uintptr_t woker_num);

  [[nodiscard]] auto Register(runtime::Event& ev, runtime::Interest interest)
      -> io::IoResult<void> override;

  [[nodiscard]] auto reregister(runtime::Event& ev, runtime::Interest interest)
      -> io::IoResult<void> override;

  [[nodiscard]] auto deregister(runtime::Event& ev, runtime::Interest interest)
      -> io::IoResult<void> override;

  [[nodiscard]] auto select(runtime::Events& events, int timeout)
      -> io::IoResult<void> override;

 private:
  runtime::Events events_;
  std::mutex mutex_;
  runtime::BlockingPool pool_;
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

#endif  // XYCO_RUNTIME_DRIVER_H_