#ifndef XYCO_RUNTIME_RUNTIME_CTX_H_
#define XYCO_RUNTIME_RUNTIME_CTX_H_

#include "driver.h"
#include "registry.h"

namespace xyco::runtime {
class Runtime;
class Driver;

class RuntimeBridge {
  friend class RuntimeCtx;
  friend class Driver;

 public:
  auto register_future(FutureBase *future) -> void;

  auto driver() -> Driver &;

  auto get_runtime() -> Runtime *;

 private:
  auto wake(Events &events) -> void;

  auto wake_local(Events &events) -> void;

  RuntimeBridge(Runtime *runtime);

  Runtime *runtime_;
};

// This class is mainly used for library inner interaction with `Runtime`. So
// it's an useful interface for expert users who neeeds to extend the library,
// like writing custom-made `Registry`. For major library callers, use `Runtime`
// and interact with it via its `spawn` interface.
class RuntimeCtx {
 public:
  static auto is_in_ctx() -> bool;

  static auto set_ctx(Runtime *runtime) -> void;

  static auto get_ctx() -> RuntimeBridge *;

 private:
  thread_local static RuntimeBridge runtime_;
};
}  // namespace xyco::runtime

#endif  // XYCO_RUNTIME_RUNTIME_CTX_H_
