#ifndef XYCO_IO_UTILS_H_
#define XYCO_IO_UTILS_H_

#include <format>

#include "utils/result.h"

namespace xyco::io {
template <typename Container>
concept Buffer = requires(Container container) {
  std::size(container);
  std::begin(container);
  std::end(container);
};

template <typename Container>
concept DynamicBuffer = requires(Container container) {
  requires Buffer<Container>;
  container.resize(0);
};

enum class Shutdown { Read, Write, All };
}  // namespace xyco::io

template <>
struct std::formatter<xyco::io::Shutdown> : public std::formatter<std::string> {
  template <typename FormatContext>
  auto format(const xyco::io::Shutdown& shutdown, FormatContext& ctx) const
      -> decltype(ctx.out()) {
    std::string shutdown_type;
    switch (shutdown) {
      case xyco::io::Shutdown::Read:
        shutdown_type = "Read";
        break;
      case xyco::io::Shutdown::Write:
        shutdown_type = "Write";
        break;
      case xyco::io::Shutdown::All:
        shutdown_type = "All";
        break;
    }
    return std::format_to(ctx.out(), "Shutdown{{{}}}", shutdown_type);
  }
};

#endif  // XYCO_IO_UTILS_H_
