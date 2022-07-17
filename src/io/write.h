#ifndef XYCO_IO_WRITE_H_
#define XYCO_IO_WRITE_H_

#include <concepts>
#include <span>

#include "io/utils.h"
#include "runtime/future.h"
#include "utils/error.h"

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
  static auto write(Writer &writer, const B &buffer)
      -> runtime::Future<utils::Result<uintptr_t>>
  requires(Writable<Writer, decltype(std::begin(buffer))> &&Buffer<B>) {
    co_return co_await writer.write(std::begin(buffer), std::end(buffer));
  }

  template <typename Writer, typename V, std::size_t N>
  // NOLINTNEXTLINE(cppcoreguidelines-avoid-c-arrays,,modernize-avoid-c-arrays)
  static auto write(Writer &writer, const V (&buffer)[N])
      -> runtime::Future<utils::Result<uintptr_t>>
  requires(Writable<Writer, decltype(std::begin(buffer))>
               &&Buffer<decltype(buffer)>) {
    auto span = std::span(buffer);
    co_return co_await write(writer, span);
  }

  template <typename Writer, typename B>
  static auto write_all(Writer &writer, const B &buffer)
      -> runtime::Future<utils::Result<void>>
  requires(Writable<Writer, decltype(std::begin(buffer))> &&Buffer<B>) {
    auto buf_size = std::size(buffer);
    auto total_write = 0;
    auto begin = std::begin(buffer);
    auto end = std::end(buffer);

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
          co_return utils::Result<void>::err(error);
        }
      }
    }
    co_return utils::Result<void>::ok();
  }

  template <typename Writer, typename V, std::size_t N>
  // NOLINTNEXTLINE(cppcoreguidelines-avoid-c-arrays,,modernize-avoid-c-arrays)
  static auto write_all(Writer &writer, const V (&buffer)[N])
      -> runtime::Future<utils::Result<void>>
  requires(Writable<Writer, decltype(std::begin(buffer))>
               &&Buffer<decltype(buffer)>) {
    auto span = std::span(buffer);
    co_return co_await write_all(writer, span);
  }
};
}  // namespace xyco::io

#endif  // XYCO_IO_WRITE_H_