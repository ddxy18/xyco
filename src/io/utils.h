#ifndef XYCO_IO_UTILS_H_
#define XYCO_IO_UTILS_H_

#include "spdlog/fmt/fmt.h"
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
struct fmt::formatter<xyco::io::Shutdown> : public fmt::formatter<std::string> {
  template <typename FormatContext>
  auto format(const xyco::io::Shutdown& shutdown, FormatContext& ctx) const
      -> decltype(ctx.out());
};

#endif  // XYCO_IO_UTILS_H_