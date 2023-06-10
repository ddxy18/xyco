#include "utils.h"

#include "io/registry.h"
#include "runtime/blocking.h"
#include "time/driver.h"

std::unique_ptr<xyco::runtime::Runtime> TestRuntimeCtx::runtime_;

TestRuntimeCtxGuard::TestRuntimeCtxGuard(
    gsl::owner<std::function<xyco::runtime::Future<void>()> *> co_wrapper,
    bool in_runtime)
    : co_wrapper_(co_wrapper) {
  auto future = (*co_wrapper_)();
  if (in_runtime) {
    future.get_handle().resume();
  }
}

TestRuntimeCtxGuard::~TestRuntimeCtxGuard() {
  xyco::runtime::RuntimeCtx::set_ctx(nullptr);
  delete co_wrapper_;
}

auto TestRuntimeCtx::init() -> void {
  runtime_ = xyco::runtime::Builder::new_multi_thread()
                 .worker_threads(1)
                 .registry<xyco::runtime::BlockingRegistry>(1)
                 .registry<xyco::io::IoRegistry>(4)
                 .registry<xyco::time::TimeRegistry>()
                 .on_worker_start([]() {})
                 .on_worker_stop([]() {})
                 .build()
                 .unwrap();
}
