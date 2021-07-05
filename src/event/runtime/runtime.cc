#include "runtime.h"

#include <memory>

#include "event/io/utils.h"
#include "event/runtime/driver.h"
#include "event/runtime/future.h"
#include "utils/result.h"

thread_local runtime::RuntimeBase *runtime::runtime = nullptr;

runtime::Worker::Worker(Runtime *runtime) : runtime_(runtime) {}

auto runtime::Worker::run() -> void {
  runtime::runtime = runtime_;
  while (true) {
    while (!runtime_->handles_.empty()) {
      runtime_->mutex_.lock();
      auto [handle, future] = runtime_->handles_.back();
      runtime_->handles_.pop_back();
      runtime_->mutex_.unlock();
      if (future == nullptr || future->poll_wrapper()) {
        handle.resume();
      }
    }
    runtime_->driver_.poll();
  }
}

auto runtime::Runtime::run() -> void {
  for (auto worker : workers_) {
    auto t = std::thread(&Worker::run, worker);
    t.detach();
  }
}

auto runtime::Builder::build() const -> IoResult<std::unique_ptr<Runtime>> {
  return Ok<std::unique_ptr<Runtime>, IoError>(
      std::make_unique<Runtime>(worker_num_, blocking_num_));
}