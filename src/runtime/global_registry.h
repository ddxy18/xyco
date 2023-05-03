#ifndef XYCO_RUNTIME_GLOBAL_REGISTRY_H_
#define XYCO_RUNTIME_GLOBAL_REGISTRY_H_

#include <gsl/pointers>

#include "runtime/runtime.h"

namespace xyco::runtime {
template <typename R>
  requires(std::derived_from<R, Registry>)
class GlobalRegistry {
 public:
  template <typename... Args>
  static auto get_instance(Args... args) -> std::shared_ptr<R> {
    auto current_runtime = gsl::make_not_null(runtime::RuntimeCtx::get_ctx());
    if (per_runtime_registries_.contains(current_runtime)) {
      return per_runtime_registries_[current_runtime];
    }
    auto registry = std::make_shared<R>(args...);
    per_runtime_registries_[current_runtime] = registry;
    return registry;
  }

 private:
  static std::unordered_map<runtime::Runtime *, std::shared_ptr<R>>
      per_runtime_registries_;
};

template <typename R>
  requires(std::derived_from<R, Registry>)
std::unordered_map<runtime::Runtime *, std::shared_ptr<R>>
    GlobalRegistry<R>::per_runtime_registries_;
}  // namespace xyco::runtime

#endif  // XYCO_RUNTIME_GLOBAL_REGISTRY_H_
