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

template <typename Reader, typename Buffer>
concept BufferReadable = requires(Reader reader) {
  {
    reader.fill_buffer()
    } -> std::same_as<runtime::Future<io::IoResult<
        std::pair<typename Buffer::iterator, typename Buffer::iterator>>>>;
  { reader.consume(0) } -> std::same_as<void>;
};

class ReadExt {
 public:
  template <typename Reader, typename B>
  static auto read(Reader &reader, B &buffer)
      -> runtime::Future<IoResult<uintptr_t>>
  requires(Readable<Reader, typename B::iterator> &&Buffer<B>) {
    co_return co_await reader.read(buffer.begin(), buffer.end());
  }

  template <typename Reader, typename B>
  static auto read_to_end(Reader &reader) -> runtime::Future<IoResult<B>>
  requires(Readable<Reader, typename B::iterator> &&Buffer<B>) {
    const int dst_init_size = 32;

    B dst(dst_init_size, 0);
    auto next_it = dst.begin();
    auto end_it = next_it + dst_init_size;
    auto dst_size = 0;

    while (true) {
      auto read_result =
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
          co_return IoResult<B>::err(error);
        }
      }
      if (read_result.unwrap() == 0) {
        break;
      }
    }
    dst.resize(dst_size);

    co_return IoResult<B>::ok(dst);
  }
};

class BufferReadExt {
 public:
  template <typename Reader, typename B>
  static auto read_until(Reader &reader, char c) -> runtime::Future<IoResult<B>>
  requires(BufferReadable<Reader, B> &&Buffer<B>) {
    B line;

    while (true) {
      auto [begin, end] = (co_await reader.fill_buffer()).unwrap();
      if (begin == end) {
        co_return IoResult<B>::ok(line);
      }
      auto pos = std::find(begin, end, c);
      if (pos != end) {
        line.insert(line.end(), begin, pos + 1);
        reader.consume(std::distance(begin, pos + 1));
        co_return IoResult<B>::ok(line);
      }
      reader.consume(std::distance(begin, pos));
      line.insert(line.end(), begin, pos);
    }
  }

  template <typename Reader, typename B>
  static auto read_line(Reader &reader) -> runtime::Future<IoResult<B>>
  requires(BufferReadable<Reader, B> &&Buffer<B>) {
    co_return(co_await read_until<Reader, B>(reader, '\n')).map([](auto line) {
      line.erase(line.end() - 1);
      return line;
    });
  }
};
}  // namespace xyco::io

#endif  // XYCO_IO_READ_H_