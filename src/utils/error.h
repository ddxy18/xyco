#ifndef XYCO_UTILS_ERROR_H_
#define XYCO_UTILS_ERROR_H_

#include "spdlog/fmt/fmt.h"
#include "utils/result.h"

namespace xyco::utils {
class Error;

template <typename T>
using Result = Result<T, Error>;

enum class ErrorKind : int { Uncategorized = -2, Unsupported };

class Error {
 public:
  static auto from_sys_error() -> Error;

  int errno_;
  std::string info_;
};

auto into_sys_result(int return_value) -> Result<int>;
}  // namespace xyco::utils

template <>
struct fmt::formatter<xyco::utils::Error> : public fmt::formatter<bool> {
  template <typename FormatContext>
  auto format(const xyco::utils::Error& err, FormatContext& ctx) const
      -> decltype(ctx.out());
};

#endif  // XYCO_UTILS_ERROR_H_