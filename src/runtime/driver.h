#ifndef XYCO_RUNTIME_DRIVER_H_
#define XYCO_RUNTIME_DRIVER_H_

#include <thread>
#include <unordered_map>

#include "blocking.h"
#include "io/driver.h"
#include "registry.h"
#include "time/driver.h"

namespace xyco::runtime {
class Driver {
 public:
  auto poll() -> void;

  auto io_handle() -> GlobalRegistry *;

  auto time_handle() -> Registry *;

  auto blocking_handle() -> Registry *;

  auto local_handle() -> Registry *;

  auto add_registry(std::unique_ptr<Registry> registry) -> void;

  explicit Driver(uintptr_t blocking_num);

 private:
  io::IoRegistry io_registry_;
  time::TimeRegistry time_registry_;
  BlockingRegistry blocking_registry_;

  std::unordered_map<std::thread::id, std::unique_ptr<Registry>>
      local_registries_;
};
}  // namespace xyco::runtime

#endif  // XYCO_RUNTIME_DRIVER_H_