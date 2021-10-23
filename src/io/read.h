#ifndef XYCO_IO_READ_H_
#define XYCO_IO_READ_H_

#include <concepts>

#include "io/utils.h"
#include "runtime/future.h"

namespace xyco::io {
template <typename Reader, typename Iterator>
concept Readable = requires(Reader reader, Iterator begin, Iterator end) {
  {
    reader.read(begin, end)
    } -> std::same_as<runtime::Future<IoResult<uintptr_t>>>;
};

template <typename Reader, typename Iterator>
concept BufferReadable = requires(Reader reader, Iterator begin, Iterator end) {
  {
    reader.fill_buffer()
    } -> std::same_as<runtime::Future<io::IoResult<
        std::pair<std::vector<char>::iterator, std::vector<char>::iterator>>>>;
  { reader.consume(0) } -> std::same_as<void>;
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

    std::vector<char> dst(dst_init_size);
    auto next_it = dst.begin();
    auto end_it = next_it + dst_init_size;
    auto dst_size = 0;
    IoResult<uintptr_t> read_result;

    while (true) {
      read_result =
          (co_await reader.read(next_it, end_it)).map([&](auto nbytes) {
            dst_size += nbytes;
            if (dst_size == dst.size()) {
              dst.resize(3 * dst_size / 2);
            }
            next_it = dst.begin() + dst_size;
            end_it = dst.end();

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
}  // namespace xyco::io

#endif  // XYCO_IO_READ_H_