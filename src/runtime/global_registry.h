#ifndef XYCO_RUNTIME_GLOBAL_REGISTRY_H_
#define XYCO_RUNTIME_GLOBAL_REGISTRY_H_

#include <gsl/pointers>
#include <shared_mutex>

#include "runtime_ctx.h"

namespace xyco::runtime {
template <typename R>
  requires(std::derived_from<R, Registry>)
class GlobalRegistry {
 public:
  template <typename... Args>
  static auto get_instance(Args... args) -> std::shared_ptr<R> {
    auto current_runtime =
        gsl::make_not_null(runtime::RuntimeCtx::get_ctx()->get_runtime());
    {
      std::shared_lock<std::shared_mutex> lock_guard(runtime_mutex_);
      if (per_runtime_registry_.contains(current_runtime)) {
        return per_runtime_registry_[current_runtime];
      }
    }

    auto registry = std::make_shared<R>(args...);
    {
      std::scoped_lock<std::shared_mutex> lock_guard(runtime_mutex_);
      per_runtime_registry_[current_runtime] = registry;
    }
    return registry;
  }

 private:
  static std::unordered_map<runtime::Runtime *, std::shared_ptr<R>>
      per_runtime_registry_;
  // Prevents runtime level data race, worker level multithread is serialized in
  // the runtime implementation.
  static std::shared_mutex runtime_mutex_;
};

template <typename R>
  requires(std::derived_from<R, Registry>)
std::unordered_map<runtime::Runtime *, std::shared_ptr<R>>
    GlobalRegistry<R>::per_runtime_registry_;
template <typename R>
  requires(std::derived_from<R, Registry>)
std::shared_mutex GlobalRegistry<R>::runtime_mutex_;
}  // namespace xyco::runtime

#endif  // XYCO_RUNTIME_GLOBAL_REGISTRY_H_
