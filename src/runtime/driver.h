#ifndef XYCO_RUNTIME_DRIVER_H_
#define XYCO_RUNTIME_DRIVER_H_

#include "blocking.h"
#include "io/driver.h"
#include "registry.h"
#include "time/driver.h"

namespace xyco::runtime {
class Driver {
 public:
  auto poll() -> void;

  auto io_handle() -> GlobalRegistry*;

  auto time_handle() -> Registry*;

  auto blocking_handle() -> Registry*;

  explicit Driver(uintptr_t blocking_num);

 private:
  io::IoRegistry io_registry_;
  time::TimeRegistry time_registry_;
  BlockingRegistry blocking_registry_;
};
}  // namespace xyco::runtime

#endif  // XYCO_RUNTIME_DRIVER_H_