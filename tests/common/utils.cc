module;

#include <memory>

module xyco.test.utils;

import xyco.io;
import xyco.net;
import xyco.fs;
import xyco.task;
import xyco.sync;

std::unique_ptr<xyco::runtime::Runtime> TestRuntimeCtx::runtime_;

auto TestRuntimeCtx::init() -> void {
  runtime_ = *xyco::runtime::Builder::new_multi_thread()
                  .worker_threads(1)
                  .registry<xyco::task::BlockingRegistry>(1)
                  .registry<xyco::io::IoRegistry>(4)
                  .registry<xyco::time::TimeRegistry>()
                  .build();
}
