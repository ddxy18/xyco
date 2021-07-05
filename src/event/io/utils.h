#ifndef XYWEBSERVER_EVENT_IO_UTILS_H_
#define XYWEBSERVER_EVENT_IO_UTILS_H_

#include <string>

#include "fmt/format.h"
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
  constexpr auto parse(format_parse_context& ctx) -> decltype(ctx.begin()) {
    // [ctx.begin(), ctx.end()) is a character range that contains a part of
    // the format string starting from the format specifications to be parsed,
    // e.g. in
    //
    //   fmt::format("{:f} - point of interest", point{1, 2});
    //
    // the range will contain "f} - point of interest". The formatter should
    // parse specifiers until '}' or the end of the range. In this example
    // the formatter should parse the 'f' specifier and return an iterator
    // pointing to '}'.

    // Parse the presentation format and store it in the formatter:
    const auto* it = ctx.begin();
    const auto* end = ctx.end();
    // Check if reached the end of the range:
    if (it != end && *it != '}') {
      throw format_error("invalid format");
    }

    // Return an iterator past the end of the parsed range:
    return it;
  }

  // Formats the point p using the parsed format specification (presentation)
  // stored in this formatter.
  template <typename FormatContext>
  auto format(const IoError& err, FormatContext& ctx) -> decltype(ctx.out()) {
    // ctx.out() is an output iterator to write to.
    return format_to(ctx.out(), "{{errno={}, info={}}}", err.errno_,
                     fmt::join(err.info_, ""));
  }
};

auto into_sys_result(int return_value) -> IoResult<int>;

#endif  // XYWEBSERVER_EVENT_IO_UTILS_H_