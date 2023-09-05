#include "utils.h"

#include "xyco/io/registry.h"
#include "xyco/task/registry.h"
#include "xyco/time/driver.h"

std::unique_ptr<xyco::runtime::Runtime> TestRuntimeCtx::runtime_;

auto TestRuntimeCtx::init() -> void {
  runtime_ = *xyco::runtime::Builder::new_multi_thread()
                  .worker_threads(1)
                  .registry<xyco::task::BlockingRegistry>(1)
                  .registry<xyco::io::IoRegistry>(4)
                  .registry<xyco::time::TimeRegistry>()
                  .on_worker_start([]() {})
                  .on_worker_stop([]() {})
                  .build();
}
