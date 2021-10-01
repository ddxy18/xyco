#ifndef XYWEBSERVER_EVENT_IO_UTILS_H_
#define XYWEBSERVER_EVENT_IO_UTILS_H_

#include "spdlog/fmt/fmt.h"
#include "utils/result.h"

class IoError;

template <typename T>
using IoResult = Result<T, IoError>;

class IoError {
 public:
  static auto from_sys_error() -> IoError;

  int errno_;
  std::string info_;
};

template <>
struct fmt::formatter<IoError> {
  static constexpr auto parse(format_parse_context& ctx)
      -> decltype(ctx.begin()) {
    const auto* it = ctx.begin();
    const auto* end = ctx.end();
    if (it != end && *it != '}') {
      throw format_error("invalid format");
    }

    return it;
  }

  template <typename FormatContext>
  auto format(const IoError& err, FormatContext& ctx) -> decltype(ctx.out()) {
    return format_to(ctx.out(), "{{errno={}, info={}}}", err.errno_,
                     fmt::join(err.info_, ""));
  }
};

auto into_sys_result(int return_value) -> IoResult<int>;

#endif  // XYWEBSERVER_EVENT_IO_UTILS_H_