#ifndef XYCO_RUNTIME_THREAD_LOCAL_REGISTRY_H_
#define XYCO_RUNTIME_THREAD_LOCAL_REGISTRY_H_

#include "xyco/runtime/registry.h"

namespace xyco::runtime {
template <typename R>
  requires(std::derived_from<R, Registry>)
class ThreadLocalRegistry {
 public:
  template <typename... Args>
  static auto get_instance(Args... args) -> std::shared_ptr<R> {
    return std::make_shared<R>(args...);
  }
};
}  // namespace xyco::runtime

#endif  // XYCO_RUNTIME_THREAD_LOCAL_REGISTRY_H_
