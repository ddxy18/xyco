#ifndef XYCO_RUNTIME_REGISTRY_H_
#define XYCO_RUNTIME_REGISTRY_H_

#include <chrono>

#include "io/utils.h"

namespace xyco::runtime {
class FutureBase;
class Registry;
class Event;
using Events = std::vector<std::reference_wrapper<Event>>;

class Extra {
 public:
  [[nodiscard]] virtual auto print() const -> std::string = 0;
};

class Event {
 public:
  runtime::FutureBase *future_{};
  Extra *extra_{};
};

class Registry {
 public:
  [[nodiscard]] virtual auto Register(Event &ev) -> io::IoResult<void> = 0;

  [[nodiscard]] virtual auto reregister(Event &ev) -> io::IoResult<void> = 0;

  [[nodiscard]] virtual auto deregister(Event &ev) -> io::IoResult<void> = 0;

  [[nodiscard]] virtual auto select(Events &events,
                                    std::chrono::milliseconds timeout)
      -> io::IoResult<void> = 0;

  Registry() = default;

  Registry(const Registry &) = delete;

  Registry(Registry &&) = delete;

  auto operator=(const Registry &) -> Registry & = delete;

  auto operator=(Registry &&) -> Registry & = delete;

  virtual ~Registry() = default;
};

class GlobalRegistry : public Registry {
 public:
  [[nodiscard]] auto Register(Event &ev) -> io::IoResult<void> override = 0;

  [[nodiscard]] auto reregister(Event &ev) -> io::IoResult<void> override = 0;

  [[nodiscard]] auto deregister(Event &ev) -> io::IoResult<void> override = 0;

  [[nodiscard]] virtual auto register_local(Event &ev)
      -> io::IoResult<void> = 0;

  [[nodiscard]] virtual auto reregister_local(Event &ev)
      -> io::IoResult<void> = 0;

  [[nodiscard]] virtual auto deregister_local(Event &ev)
      -> io::IoResult<void> = 0;

  [[nodiscard]] auto select(Events &events, std::chrono::milliseconds timeout)
      -> io::IoResult<void> override = 0;
};
}  // namespace xyco::runtime

template <>
struct fmt::formatter<xyco::runtime::Event> : public fmt::formatter<bool> {
  template <typename FormatContext>
  auto format(const xyco::runtime::Event &event, FormatContext &ctx) const
      -> decltype(ctx.out());
};

#endif  // XYCO_RUNTIME_REGISTRY_H_