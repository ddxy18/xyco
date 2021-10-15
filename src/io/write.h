#ifndef XYCO_IO_WRITE_H_
#define XYCO_IO_WRITE_H_

#include <concepts>

#include "io/utils.h"
#include "runtime/future.h"

namespace xyco::io {
enum class Shutdown { Read, Write, All };

template <typename Writer, typename Iterator>
concept Writable = requires(Writer writer, Iterator begin, Iterator end) {
  {
    writer.write(begin, end)
    } -> std::same_as<runtime::Future<IoResult<uintptr_t>>>;

  { writer.flush() } -> std::same_as<runtime::Future<IoResult<void>>>;

  {
    writer.shutdown(io::Shutdown::All)
    } -> std::same_as<runtime::Future<IoResult<void>>>;
};

template <typename Writer>
class WriteExt {
 public:
  static auto write(Writer &writer, const std::vector<char> &buf)
      -> runtime::Future<IoResult<uintptr_t>>
  requires(Writable<Writer, decltype(buf.cbegin())>) {
    co_return co_await writer.write(buf.cbegin(), buf.cend());
  }

  static auto write_all(Writer &writer, const std::vector<char> &buf)
      -> runtime::Future<IoResult<void>>
  requires(Writable<Writer, decltype(buf.cbegin())>) {
    auto buf_size = buf.size();
    auto total_write = 0;
    auto begin = buf.cbegin();
    auto end = buf.cend();

    while (total_write != buf_size) {
      auto write_result =
          (co_await writer.write(begin, end)).map([&](auto nbytes) {
            total_write += nbytes;
            begin += nbytes;
          });
      if (write_result.is_err()) {
        auto error = write_result.unwrap_err();
        if (error.errno_ != EAGAIN && error.errno_ != EWOULDBLOCK &&
            error.errno_ != EINTR) {
          co_return IoResult<void>::err(error);
        }
      }
    }
    co_return IoResult<void>::ok();
  }
};
}  // namespace xyco::io

#endif  // XYCO_IO_WRITE_H_