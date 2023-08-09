#ifndef XYCO_RUNTIME_DRIVER_H_
#define XYCO_RUNTIME_DRIVER_H_

#include <functional>
#include <thread>
#include <unordered_map>

#include "xyco/runtime/registry.h"

namespace xyco::runtime {
class Runtime;

class Driver {
 public:
  auto poll() -> void;

  template <typename R>
  auto Register(std::shared_ptr<Event> event) -> void {
    *local_registries_.find(std::this_thread::get_id())
         ->second.find(typeid(R).hash_code())
         ->second->Register(std::move(event));
  }

  template <typename R>
  auto reregister(std::shared_ptr<Event> event) -> void {
    *local_registries_.find(std::this_thread::get_id())
         ->second.find(typeid(R).hash_code())
         ->second->reregister(std::move(event));
  }

  template <typename R>
  auto deregister(std::shared_ptr<Event> event) -> void {
    *local_registries_.find(std::this_thread::get_id())
         ->second.find(typeid(R).hash_code())
         ->second->deregister(std::move(event));
  }

  template <typename R, typename... Args>
  auto add_registry(Args... args) -> void {
    local_registries_[std::this_thread::get_id()][typeid(R).hash_code()] =
        R::get_instance(args...);
  }

  auto add_thread() -> void;

  Driver(std::vector<std::function<void(Runtime*)>>&& registry_initializers)
      : registry_initializers_(std::move(registry_initializers)) {}

 private:
  constexpr static std::chrono::milliseconds MAX_TIMEOUT =
      std::chrono::milliseconds(2);

  std::vector<std::function<void(Runtime*)>> registry_initializers_;

  std::unordered_map<std::thread::id,
                     std::unordered_map<decltype(typeid(int).hash_code()),
                                        std::shared_ptr<Registry>>>
      local_registries_;
};
}  // namespace xyco::runtime

#endif  // XYCO_RUNTIME_DRIVER_H_
