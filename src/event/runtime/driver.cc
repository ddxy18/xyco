#include "event/net/epoll.h"
#include "fmt/core.h"
#include "runtime.h"

auto runtime::Driver::poll() -> void {
  net_poll_.poll(&events_, net::Epoll::MAX_TIMEOUT);
  blocking_poll_.poll(&events_, net::Epoll::MAX_TIMEOUT);
}