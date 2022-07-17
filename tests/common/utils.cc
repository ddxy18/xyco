#include "utils.h"

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