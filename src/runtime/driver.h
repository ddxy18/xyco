#ifndef XYCO_RUNTIME_DRIVER_H_
#define XYCO_RUNTIME_DRIVER_H_

#include <mutex>
#include <thread>
#include <unordered_map>

#include "registry.h"

namespace xyco::runtime {
class Driver {
 public:
  auto poll() -> void;

  // a lockfree version of 'register', 'reregister' and 'select'
  template <typename R>
  auto handle() -> Registry* {
    return registries_.find(typeid(R).hash_code())->second.get();
  }

  template <typename R>
  auto Register(std::shared_ptr<Event> event) -> void {
    std::scoped_lock<std::mutex> lock_guard(register_mutex_);
    register_events_.emplace(typeid(R).hash_code(), event);
  }

  template <typename R>
  auto reregister(std::shared_ptr<Event> event) -> void {
    std::scoped_lock<std::mutex> lock_guard(reregister_mutex_);
    reregister_events_.emplace(typeid(R).hash_code(), event);
  }

  template <typename R>
  auto deregister(std::shared_ptr<Event> event) -> void {
    std::scoped_lock<std::mutex> lock_guard(mutexes_[typeid(R).hash_code()]);
    registries_.find(typeid(R).hash_code())
        ->second->deregister(std::move(event))
        .unwrap();
  }

  auto dispatch() -> void;

  template <typename R>
  auto local_handle() -> Registry* {
    return local_registries_.find(std::this_thread::get_id())
        ->second.find(typeid(R).hash_code())
        ->second.get();
  }

  template <typename R, typename... Args>
  auto add_registry(Args... args) -> void {
    registries_[typeid(R).hash_code()] = std::make_unique<R>(args...);
  }

  template <typename R, typename... Args>
  auto add_local_registry(Args... args) -> void {
    local_registries_[std::this_thread::get_id()][typeid(R).hash_code()] =
        std::make_unique<R>(args...);
  }

  auto add_thread() -> void;

 private:
  constexpr static std::chrono::milliseconds MAX_TIMEOUT =
      std::chrono::milliseconds(2);

  std::unordered_map<decltype(typeid(int).hash_code()),
                     std::unique_ptr<Registry>>
      registries_;
  std::unordered_map<decltype(typeid(int).hash_code()), std::mutex> mutexes_;

  std::unordered_map<std::thread::id,
                     std::unordered_map<decltype(typeid(int).hash_code()),
                                        std::unique_ptr<Registry>>>
      local_registries_;

  thread_local static std::unordered_map<decltype(typeid(int).hash_code()),
                                         std::shared_ptr<Event>>
      register_events_;
  std::mutex register_mutex_;
  thread_local static std::unordered_map<decltype(typeid(int).hash_code()),
                                         std::shared_ptr<Event>>
      reregister_events_;
  std::mutex reregister_mutex_;
};
}  // namespace xyco::runtime

#endif  // XYCO_RUNTIME_DRIVER_H_