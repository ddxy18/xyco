module;

#include <expected>
#include <functional>
#include <vector>

module xyco.runtime;

xyco::runtime::Runtime::Runtime(
    std::vector<std::function<void(Driver *)>> &&registry_initializers,
    uint16_t worker_num)
    : core_(std::move(registry_initializers), worker_num) {}

auto xyco::runtime::Builder::new_multi_thread() -> Builder { return {}; }

auto xyco::runtime::Builder::worker_threads(uint16_t val) -> Builder & {
  worker_num_ = val;
  return *this;
}

auto xyco::runtime::Builder::build()
    -> std::expected<std::unique_ptr<Runtime>, std::nullptr_t> {
  return std::make_unique<Runtime>(std::move(registry_initializers_),
                                   worker_num_);
}
