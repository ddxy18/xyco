#ifndef XYWEBSERVER_EVENT_RUNTIME_RUNTIME_BASE_H_
#define XYWEBSERVER_EVENT_RUNTIME_RUNTIME_BASE_H_

namespace reactor {
class Poll;
}

namespace runtime {
class FutureBase;
using IoHandle = reactor::Poll;

class RuntimeBase {
 public:
  virtual auto register_future(FutureBase *future) -> void = 0;
  virtual auto io_handle() -> IoHandle * = 0;
  virtual auto blocking_handle() -> IoHandle * = 0;
};
}  // namespace runtime

#endif  // XYWEBSERVER_EVENT_RUNTIME_RUNTIME_BASE_H_