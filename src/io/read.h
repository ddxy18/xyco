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

  static auto read_to_end(Reader &reader)
      -> runtime::Future<IoResult<std::vector<char>>>
  requires(Readable<Reader, std::vector<char>::iterator>) {
    const int dst_init_size = 32;

    std::vector<char> dst;
    dst.reserve(dst_init_size);
    auto next_it = dst.begin();
    auto end_it = next_it + dst_init_size;
    decltype(dst.size()) dst_size = 0;
    IoResult<uintptr_t> read_result;

    while (true) {
      read_result =
          (co_await reader.read(next_it, end_it)).map([&](auto nbytes) {
            dst_size += nbytes;
            dst.reserve(dst_size + nbytes);
            next_it = dst.end();
            end_it = next_it + nbytes;

            return nbytes;
          });
      if (read_result.is_err()) {
        auto error = read_result.unwrap_err();
        if (error.errno_ != EAGAIN && error.errno_ != EWOULDBLOCK &&
            error.errno_ != EINTR) {
          co_return IoResult<std::vector<char>>::err(error);
        }
      }
      if (read_result.unwrap() == 0) {
        break;
      }
    }
    dst.resize(dst_size);

    co_return IoResult<std::vector<char>>::ok(dst);
  }
};
}  // namespace io

#endif  // XYCO_IO_READ_H_