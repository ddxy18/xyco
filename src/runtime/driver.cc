#include "driver.h"

#include "runtime.h"

thread_local std::unordered_map<decltype(typeid(int).hash_code()),
                                std::shared_ptr<xyco::runtime::Event>>
    xyco::runtime::Driver::register_events_;

thread_local std::unordered_map<decltype(typeid(int).hash_code()),
                                std::shared_ptr<xyco::runtime::Event>>
    xyco::runtime::Driver::reregister_events_;

auto xyco::runtime::Driver::poll() -> void {
  runtime::Events events;

  auto& local_registry =
      local_registries_.find(std::this_thread::get_id())->second;
  for (auto& [key, registry] : local_registry) {
    registry->select(events, MAX_TIMEOUT).unwrap();
    RuntimeCtx::get_ctx()->wake_local(events);
  }

  for (auto& [key, registry] : registries_) {
    std::scoped_lock<std::mutex> lock_guard(mutexes_[key]);
    registry->select(events, MAX_TIMEOUT).unwrap();
    RuntimeCtx::get_ctx()->wake(events);
  }
}

auto xyco::runtime::Driver::dispatch() -> void {
  {
    std::scoped_lock<std::mutex> lock_guard(register_mutex_);
    for (auto&& pair : register_events_) {
      std::scoped_lock<std::mutex> lock_guard(mutexes_[pair.first]);
      registries_.find(pair.first)->second->Register(pair.second).unwrap();
    }
    register_events_.clear();
  }
  {
    std::scoped_lock<std::mutex> lock_guard(reregister_mutex_);
    for (auto&& pair : reregister_events_) {
      std::scoped_lock<std::mutex> lock_guard(mutexes_[pair.first]);
      registries_.find(pair.first)->second->reregister(pair.second).unwrap();
    }
    reregister_events_.clear();
  }
}

auto xyco::runtime::Driver::add_thread() -> void {
  local_registries_[std::this_thread::get_id()] = std::remove_reference_t<
      decltype(local_registries_[std::this_thread::get_id()])>();
  for (auto& [key, registry] : registries_) {
    auto* global_registry = dynamic_cast<GlobalRegistry*>(registry.get());
    if (global_registry != nullptr) {
      global_registry->local_registry_init();
    }
  }
}