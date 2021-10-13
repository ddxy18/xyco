#ifndef XYCO_IO_READ_H_
#define XYCO_IO_READ_H_

#include "io/utils.h"
#include "runtime/future.h"

namespace io {
template <typename Reader, typename Iterator>
concept Readable = requires(Reader reader, Iterator begin, Iterator end) {
  {
    reader.read(begin, end)
    } -> std::same_as<runtime::Future<IoResult<uintptr_t>>>;
};

template <typename Reader>
class ReadExt {
 public:
  static auto read(Reader &reader, std::vector<char> &buf)
      -> runtime::Future<IoResult<uintptr_t>>
  requires(Readable<Reader, decltype(buf.begin())>) {
    co_return co_await reader.read(buf.begin(), buf.end());
  }
};
}  // namespace io

#endif  // XYCO_IO_READ_H_
