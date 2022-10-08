#ifndef XYCO_TESTS_COMMON_EPOLL_UTILS_H_
#define XYCO_TESTS_COMMON_EPOLL_UTILS_H_

#include "common/utils.h"
#include "fs/epoll/file.h"
#include "io/epoll/extra.h"
#include "net/epoll/listener.h"

namespace xyco::fs {
using namespace epoll;
}

namespace xyco::io {
using namespace epoll;
}

namespace xyco::net {
using namespace epoll;
}

#endif  // XYCO_TESTS_COMMON_EPOLL_UTILS_H_