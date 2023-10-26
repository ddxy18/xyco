#ifndef XYCO_IO_WRITE_H_
#define XYCO_IO_WRITE_H_

#include <expected>
#include <span>

#include "xyco/io/utils.h"

import xyco.error;
import xyco.future;

namespace xyco::io {
template <typename Writer, typename Iterator>
concept Writable = requires(Writer writer, Iterator begin, Iterator end) {
  {
    writer.write(begin, end)
  } -> std::same_as<runtime::Future<utils::Result<uintptr_t>>>;

  { writer.flush() } -> std::same_as<runtime::Future<utils::Result<void>>>;

  {
    writer.shutdown(io::Shutdown::All)
  } -> std::same_as<runtime::Future<utils::Result<void>>>;
};

class WriteExt {
 public:
  template <typename Writer, typename B>
  // NOLINTNEXTLINE(cppcoreguidelines-avoid-reference-coroutine-parameters)
  static auto write(Writer &writer, const B &buffer)
      -> runtime::Future<utils::Result<uintptr_t>>
    requires(Writable<Writer, decltype(std::begin(buffer))> && Buffer<B>)
  {
    co_return co_await writer.write(std::begin(buffer), std::end(buffer));
  }

  template <typename Writer, typename V, std::size_t N>
  // NOLINTNEXTLINE(cppcoreguidelines-avoid-c-arrays,modernize-avoid-c-arrays,cppcoreguidelines-avoid-reference-coroutine-parameters)
  static auto write(Writer &writer, const V (&buffer)[N])
      -> runtime::Future<utils::Result<uintptr_t>>
    requires(Writable<Writer, decltype(std::begin(buffer))> &&
             Buffer<decltype(buffer)>)
  {
    auto span = std::span(buffer);
    co_return co_await write(writer, span);
  }

  template <typename Writer, typename B>
  // NOLINTNEXTLINE(cppcoreguidelines-avoid-reference-coroutine-parameters)
  static auto write_all(Writer &writer, const B &buffer)
      -> runtime::Future<utils::Result<void>>
    requires(Writable<Writer, decltype(std::begin(buffer))> && Buffer<B>)
  {
    auto buf_size = std::size(buffer);
    unsigned long total_write = 0;
    auto begin = std::begin(buffer);
    auto end = std::end(buffer);

    while (total_write != buf_size) {
      auto write_result =
          (co_await writer.write(begin, end)).transform([&](auto nbytes) {
            total_write += nbytes;
            begin += nbytes;
          });
      if (!write_result) {
        auto error = write_result.error();
        if (error.errno_ != EAGAIN && error.errno_ != EWOULDBLOCK &&
            error.errno_ != EINTR) {
          co_return std::unexpected(error);
        }
      }
    }
    co_return {};
  }

  template <typename Writer, typename V, std::size_t N>
  // NOLINTNEXTLINE(cppcoreguidelines-avoid-c-arrays,modernize-avoid-c-arrays,cppcoreguidelines-avoid-reference-coroutine-parameters)
  static auto write_all(Writer &writer, const V (&buffer)[N])
      -> runtime::Future<utils::Result<void>>
    requires(Writable<Writer, decltype(std::begin(buffer))> &&
             Buffer<decltype(buffer)>)
  {
    auto span = std::span(buffer);
    co_return co_await write_all(writer, span);
  }
};
}  // namespace xyco::io

#endif  // XYCO_IO_WRITE_H_
