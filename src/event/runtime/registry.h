#ifndef XYWEBSERVER_EVENT_RUNTIME_REGISTRY_H_
#define XYWEBSERVER_EVENT_RUNTIME_REGISTRY_H_

#include "event/io/utils.h"

namespace runtime {
class FutureBase;
}

namespace reactor {
class Registry;
class Event;
enum class Interest;
using Events = std::vector<Event *>;

enum class Interest { Read, Write, All };

class Event {
 public:
  Interest interest_{};
  int fd_{};
  runtime::FutureBase *future_{};
  std::function<void()> before_extra_;
  void *after_extra_{};
};

class Registry {
 public:
  [[nodiscard]] virtual auto Register(Event *ev) -> IoResult<void> = 0;

  [[nodiscard]] virtual auto reregister(Event *ev) -> IoResult<void> = 0;

  [[nodiscard]] virtual auto deregister(Event *ev) -> IoResult<void> = 0;

  [[nodiscard]] virtual auto select(Events *events, int timeout)
      -> IoResult<void> = 0;

  Registry() = default;

  Registry(const Registry &) = delete;

  Registry(Registry &&) = delete;

  auto operator=(const Registry &) -> Registry & = delete;

  auto operator=(Registry &&) -> Registry & = delete;

  virtual ~Registry() = default;
};

class GlobalRegistry : public Registry {
 public:
  [[nodiscard]] auto Register(Event *ev) -> IoResult<void> override = 0;

  [[nodiscard]] auto reregister(Event *ev) -> IoResult<void> override = 0;

  [[nodiscard]] auto deregister(Event *ev) -> IoResult<void> override = 0;

  [[nodiscard]] virtual auto register_local(Event *ev) -> IoResult<void> = 0;

  [[nodiscard]] virtual auto reregister_local(Event *ev) -> IoResult<void> = 0;

  [[nodiscard]] virtual auto deregister_local(Event *ev) -> IoResult<void> = 0;

  [[nodiscard]] auto select(Events *events, int timeout)
      -> IoResult<void> override = 0;
};
}  // namespace reactor

#endif  // XYWEBSERVER_EVENT_RUNTIME_REGISTRY_H_