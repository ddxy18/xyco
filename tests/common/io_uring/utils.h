#ifndef XYCO_TESTS_COMMON_IO_URING_UTILS_H_
#define XYCO_TESTS_COMMON_IO_URING_UTILS_H_

#include "common/utils.h"
#include "fs/io_uring/file.h"
#include "io/io_uring/extra.h"
#include "net/io_uring/listener.h"

namespace xyco::fs {
using namespace uring;
}

namespace xyco::io {
using namespace uring;
}

namespace xyco::net {
using namespace uring;
}

#endif  // XYCO_TESTS_COMMON_IO_URING_UTILS_H_