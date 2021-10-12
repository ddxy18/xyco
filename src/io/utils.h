#ifndef XYCO_IO_UTILS_H_
#define XYCO_IO_UTILS_H_

#include "spdlog/fmt/fmt.h"
#include "utils/result.h"

namespace io {
class IoError;

template <typename T>
using IoResult = Result<T, IoError>;

class IoError {
 public:
  static auto from_sys_error() -> IoError;

  int errno_;
  std::string info_;
};

auto into_sys_result(int return_value) -> IoResult<int>;
}  // namespace io

template <>
struct fmt::formatter<io::IoError> : public fmt::formatter<bool> {
  template <typename FormatContext>
  auto format(const io::IoError& err, FormatContext& ctx) const
      -> decltype(ctx.out());
};

#endif  // XYCO_IO_UTILS_H_