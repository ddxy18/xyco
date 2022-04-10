#ifndef XYCO_IO_DRIVER_H_
#define XYCO_IO_DRIVER_H_

#include "net/driver/mod.h"
#include "runtime/registry.h"

namespace xyco::io {
class IoExtra : public runtime::Extra {
 public:
  enum class State { Pending, Readable, Writable, All, Error };
  enum class Interest { Read, Write, All };

  [[nodiscard]] auto readable() const -> bool;

  [[nodiscard]] auto writeable() const -> bool;

  auto clear_readable() -> void;

  auto clear_writeable() -> void;

  [[nodiscard]] auto print() const -> std::string override;

  IoExtra(Interest interest, int fd, State state = State::Pending);

  State state_;
  Interest interest_;
  int fd_;
};

class IoRegistry : public runtime::Registry {
 public:
  [[nodiscard]] auto Register(runtime::Event& ev) -> IoResult<void> override;

  [[nodiscard]] auto reregister(runtime::Event& ev) -> IoResult<void> override;

  [[nodiscard]] auto deregister(runtime::Event& ev) -> IoResult<void> override;

  [[nodiscard]] auto select(runtime::Events& events,
                            std::chrono::milliseconds timeout)
      -> IoResult<void> override;

 private:
  net::NetRegistry registry_;
};
}  // namespace xyco::io

template <>
struct fmt::formatter<xyco::io::IoExtra> : public fmt::formatter<std::string> {
  template <typename FormatContext>
  auto format(const xyco::io::IoExtra& extra, FormatContext& ctx) const
      -> decltype(ctx.out());
};

#endif  // XYCO_IO_DRIVER_H_