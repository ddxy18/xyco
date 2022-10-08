#include "utils.h"

#include "io/epoll/epoll.h"
#include "time/driver.h"

std::unique_ptr<xyco::runtime::Runtime> TestRuntimeCtx::runtime_(
    xyco::runtime::Builder::new_multi_thread()
        .worker_threads(1)
        .max_blocking_threads(1)
        .registry<xyco::io::epoll::IoRegistry>(4)
        .registry<xyco::time::TimeRegistry>()
        .on_worker_start([]() {})
        .on_worker_stop([]() {})
        .build()
        .unwrap());
