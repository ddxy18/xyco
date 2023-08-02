#ifndef XYCO_UTILS_ERROR_H_
#define XYCO_UTILS_ERROR_H_

#include <format>

#include "xyco/utils/result.h"

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
struct std::formatter<xyco::utils::Error> : public std::formatter<bool> {
  template <typename FormatContext>
  auto format(const xyco::utils::Error& err, FormatContext& ctx) const
      -> decltype(ctx.out()) {
    if (err.errno_ > 0) {
      return std::format_to(ctx.out(), "IoError{{errno={}, info={}}}",
                            err.errno_, err.info_);
    }
    std::string error_kind;
    switch (err.errno_) {
      case std::__to_underlying(xyco::utils::ErrorKind::Uncategorized):
        error_kind = "Uncategorized";
        break;
      case std::__to_underlying(xyco::utils::ErrorKind::Unsupported):
        error_kind = "Unsupported";
        break;
      default:
        error_kind = "Unknown";
    }
    return std::format_to(ctx.out(), "IoError{{error_kind={}, info={}}}",
                          error_kind, err.info_);
  }
};

#endif  // XYCO_UTILS_ERROR_H_
