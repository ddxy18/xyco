#ifndef XYCO_RUNTIME_REGISTRY_H_
#define XYCO_RUNTIME_REGISTRY_H_

#include "utils/error.h"

namespace xyco::runtime {
class FutureBase;
class Registry;
class Event;
using Events = std::vector<std::weak_ptr<Event>>;

class Extra {
 public:
  [[nodiscard]] virtual auto print() const -> std::string = 0;

  Extra() = default;

  Extra(const Extra &extra) = delete;

  Extra(Extra &&extra) = delete;

  auto operator=(const Extra &) -> Extra & = delete;

  auto operator=(Extra &&) -> Extra & = delete;

  virtual ~Extra() = default;
};

class Event {
 public:
  runtime::FutureBase *future_{};
  std::unique_ptr<Extra> extra_;
};

class Registry {
 public:
  [[nodiscard]] virtual auto Register(std::shared_ptr<Event> event)
      -> utils::Result<void> = 0;

  [[nodiscard]] virtual auto reregister(std::shared_ptr<Event> event)
      -> utils::Result<void> = 0;

  [[nodiscard]] virtual auto deregister(std::shared_ptr<Event> event)
      -> utils::Result<void> = 0;

  [[nodiscard]] virtual auto select(Events &events,
                                    std::chrono::milliseconds timeout)
      -> utils::Result<void> = 0;

  Registry() = default;

  Registry(const Registry &) = delete;

  Registry(Registry &&) = delete;

  auto operator=(const Registry &) -> Registry & = delete;

  auto operator=(Registry &&) -> Registry & = delete;

  virtual ~Registry() = default;
};

class GlobalRegistry : public Registry {
 public:
  [[nodiscard]] auto Register(std::shared_ptr<Event> event)
      -> utils::Result<void> override = 0;

  [[nodiscard]] auto reregister(std::shared_ptr<Event> event)
      -> utils::Result<void> override = 0;

  [[nodiscard]] auto deregister(std::shared_ptr<Event> event)
      -> utils::Result<void> override = 0;

  [[nodiscard]] virtual auto register_local(std::shared_ptr<Event> event)
      -> utils::Result<void> = 0;

  [[nodiscard]] virtual auto reregister_local(std::shared_ptr<Event> event)
      -> utils::Result<void> = 0;

  [[nodiscard]] virtual auto deregister_local(std::shared_ptr<Event> event)
      -> utils::Result<void> = 0;

  [[nodiscard]] auto select(Events &events, std::chrono::milliseconds timeout)
      -> utils::Result<void> override = 0;

  virtual auto local_registry_init() -> void = 0;
};
}  // namespace xyco::runtime

template <>
struct fmt::formatter<xyco::runtime::Event> : public fmt::formatter<bool> {
  template <typename FormatContext>
  auto format(const xyco::runtime::Event &event, FormatContext &ctx) const
      -> decltype(ctx.out());
};

#endif  // XYCO_RUNTIME_REGISTRY_H_