module xyco.runtime_ctx;

auto xyco::runtime::RuntimeCtx::get_ctx() -> xyco::runtime::RuntimeCore * {
  return RuntimeCtxImpl::get_ctx();
}

auto xyco::runtime::RuntimeCtx::register_future(xyco::runtime::FutureBase *future) -> void {
  RuntimeCtxImpl::get_ctx()->register_future(future);
}

auto xyco::runtime::RuntimeCtx::driver() -> xyco::runtime::Driver & {
  return RuntimeCtxImpl::get_ctx()->driver();
}

auto xyco::runtime::RuntimeCtx::wake(xyco::runtime::Events &events) -> void {
  RuntimeCtxImpl::get_ctx()->wake(events);
}

auto xyco::runtime::RuntimeCtx::wake_local(xyco::runtime::Events &events) -> void {
  RuntimeCtxImpl::get_ctx()->wake_local(events);
}
