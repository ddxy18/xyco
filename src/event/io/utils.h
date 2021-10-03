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
struct fmt::formatter<IoError> : public fmt::formatter<bool> {
  template <typename FormatContext>
  auto format(const IoError& err, FormatContext& ctx) const
      -> decltype(ctx.out());
};

auto into_sys_result(int return_value) -> IoResult<int>;

#endif  // XYWEBSERVER_EVENT_IO_UTILS_H_