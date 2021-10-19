#ifndef XYCO_RUNTIME_REGISTRY_H_
#define XYCO_RUNTIME_REGISTRY_H_

#include <chrono>

#include "io/utils.h"

namespace xyco::runtime {
class FutureBase;
class Registry;
class Event;
enum class Interest;
using Events = std::vector<std::reference_wrapper<Event>>;

enum class Interest { Read, Write, All };

class IoExtra {
 public:
  enum class State { Pending, Readable, Writable, All };

  [[nodiscard]] auto readable() const -> bool;

  [[nodiscard]] auto writeable() const -> bool;

  auto clear_readable() -> void;

  auto clear_writeable() -> void;

  State state_;
  int fd_;
};

class TimeExtra {
 public:
  std::chrono::time_point<std::chrono::system_clock> expire_time_;
};

class AsyncFutureExtra {
 public:
  std::function<void()> before_extra_;
  void *after_extra_{};
};

class Event {
 public:
  runtime::FutureBase *future_{};
  std::variant<AsyncFutureExtra, IoExtra, TimeExtra> extra_;
};

class Registry {
 public:
  [[nodiscard]] virtual auto Register(Event &ev, Interest interest)
      -> io::IoResult<void> = 0;

  [[nodiscard]] virtual auto reregister(Event &ev, Interest interest)
      -> io::IoResult<void> = 0;

  [[nodiscard]] virtual auto deregister(Event &ev, Interest interest)
      -> io::IoResult<void> = 0;

  [[nodiscard]] virtual auto select(Events &events, int timeout)
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
  [[nodiscard]] auto Register(Event &ev, Interest interest)
      -> io::IoResult<void> override = 0;

  [[nodiscard]] auto reregister(Event &ev, Interest interest)
      -> io::IoResult<void> override = 0;

  [[nodiscard]] auto deregister(Event &ev, Interest interest)
      -> io::IoResult<void> override = 0;

  [[nodiscard]] virtual auto register_local(Event &ev, Interest interest)
      -> io::IoResult<void> = 0;

  [[nodiscard]] virtual auto reregister_local(Event &ev, Interest interest)
      -> io::IoResult<void> = 0;

  [[nodiscard]] virtual auto deregister_local(Event &ev, Interest interest)
      -> io::IoResult<void> = 0;

  [[nodiscard]] auto select(Events &events, int timeout)
      -> io::IoResult<void> override = 0;
};
}  // namespace xyco::runtime

template <>
struct fmt::formatter<xyco::runtime::IoExtra> : public fmt::formatter<bool> {
  template <typename FormatContext>
  auto format(const xyco::runtime::IoExtra &extra, FormatContext &ctx) const
      -> decltype(ctx.out());
};

template <>
struct fmt::formatter<xyco::runtime::TimeExtra> : public fmt::formatter<bool> {
  template <typename FormatContext>
  auto format(const xyco::runtime::TimeExtra &extra, FormatContext &ctx) const
      -> decltype(ctx.out());
};

template <>
struct fmt::formatter<xyco::runtime::AsyncFutureExtra>
    : public fmt::formatter<bool> {
  template <typename FormatContext>
  auto format(const xyco::runtime::AsyncFutureExtra &extra,
              FormatContext &ctx) const -> decltype(ctx.out());
};

template <>
struct fmt::formatter<xyco::runtime::Event> : public fmt::formatter<bool> {
  template <typename FormatContext>
  auto format(const xyco::runtime::Event &event, FormatContext &ctx) const
      -> decltype(ctx.out());
};

#endif  // XYCO_RUNTIME_REGISTRY_H_