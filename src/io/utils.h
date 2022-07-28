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
}  // namespace xyco::io

#endif  // XYCO_IO_UTILS_H_