#ifndef XYWEBSERVER_EVENT_MOD_H_
#define XYWEBSERVER_EVENT_MOD_H_

namespace event {
class A {};
#ifdef linux
#include "epoll.h"
#endif
}  // namespace event
#endif  // XYWEBSERVER_EVENT_MOD_H_