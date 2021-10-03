#ifndef XYWEBSERVER_EVENT_RUNTIME_POLL_H_
#define XYWEBSERVER_EVENT_RUNTIME_POLL_H_

#include "event/io/utils.h"

namespace runtime {
class FutureBase;
}

namespace reactor {
class Poll;
class Registry;
class Event;
enum class Interest;
using Events = std::vector<Event *>;

enum class Interest { Read, Write, All };

class Event {
 public:
  Interest interest_;
  int fd_;
  runtime::FutureBase *future_;
  void *before_extra_;
  void *after_extra_;
};

class Registry {
 public:
  [[nodiscard]] virtual auto Register(Event *ev) -> IoResult<Void> = 0;

  [[nodiscard]] virtual auto reregister(Event *ev) -> IoResult<Void> = 0;

  [[nodiscard]] virtual auto deregister(Event *ev) -> IoResult<Void> = 0;

  [[nodiscard]] virtual auto select(Events *events, int timeout)
      -> IoResult<Void> = 0;

  Registry() = default;

  Registry(Registry &) = default;

  Registry(Registry &&) = default;

  auto operator=(const Registry &) -> Registry & = default;

  auto operator=(Registry &&) -> Registry & = default;

  virtual ~Registry() = default;
};

class Poll {
 public:
  explicit Poll(std::unique_ptr<Registry> registry);

  auto registry() -> Registry *;

  auto poll(Events *events, int timeout) -> IoResult<Void>;

 private:
  std::unique_ptr<Registry> registry_;
};
}  // namespace reactor

#endif  // XYWEBSERVER_EVENT_RUNTIME_POLL_H_