#ifndef XYCO_IO_READ_H_
#define XYCO_IO_READ_H_

#include <concepts>
#include <span>

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
concept BufferReadable = requires(Reader reader, Buffer buffer) {
  {
    reader.fill_buffer()
    } -> std::same_as<runtime::Future<io::IoResult<std::pair<
        decltype(std::begin(buffer)), decltype(std::begin(buffer))>>>>;
  { reader.consume(0) } -> std::same_as<void>;
};

class ReadExt {
 public:
  template <typename Reader, typename B>
  static auto read(Reader &reader, B &buffer)
      -> runtime::Future<IoResult<uintptr_t>>
  requires(Readable<Reader, decltype(std::begin(buffer))> &&Buffer<B>) {
    co_return co_await reader.read(std::begin(buffer), std::end(buffer));
  }

  template <typename Reader, typename V, std::size_t N>
  // NOLINTNEXTLINE(cppcoreguidelines-avoid-c-arrays,,modernize-avoid-c-arrays)
  static auto read(Reader &reader, V (&buffer)[N])
      -> runtime::Future<IoResult<uintptr_t>>
  requires(Readable<Reader, decltype(std::begin(buffer))>
               &&Buffer<decltype(buffer)>) {
    auto span = std::span(buffer);
    co_return co_await read(reader, span);
  }
};

class BufferReadExt {
 public:
  template <typename Reader, typename B>
  static auto read_to_end(Reader &reader) -> runtime::Future<IoResult<B>>
  requires(BufferReadable<Reader, B> &&DynamicBuffer<B>) {
    B content;

    while (true) {
      auto [begin, end] = (co_await reader.fill_buffer()).unwrap();
      if (begin == end) {
        co_return IoResult<B>::ok(content);
      }
      auto prev_size = std::size(content);
      content.resize(prev_size + std::distance(begin, end));
      std::copy(begin, end, std::begin(content) + prev_size);
      reader.consume(std::distance(begin, end));
    }
  }

  template <typename Reader, typename B>
  static auto read_until(Reader &reader, char character) -> runtime::Future<IoResult<B>>
  requires(BufferReadable<Reader, B> &&DynamicBuffer<B>) {
    B content;

    while (true) {
      auto [begin, end] = (co_await reader.fill_buffer()).unwrap();
      if (begin == end) {
        co_return IoResult<B>::ok(content);
      }
      auto pos = std::find(begin, end, character);
      if (pos != end) {
        std::advance(pos, 1);
      }
      auto prev_size = std::size(content);
      content.resize(prev_size + std::distance(begin, pos));
      std::copy(begin, pos, std::begin(content) + prev_size);
      reader.consume(std::distance(begin, pos));
      if (pos != end) {
        co_return IoResult<B>::ok(content);
      }
    }
  }

  template <typename Reader, typename B>
  static auto read_line(Reader &reader) -> runtime::Future<IoResult<B>>
  requires(BufferReadable<Reader, B> &&DynamicBuffer<B>) {
    co_return(co_await read_until<Reader, B>(reader, '\n')).map([](auto line) {
      line.resize(std::size(line) - 1);
      return line;
    });
  }
};
}  // namespace xyco::io

#endif  // XYCO_IO_READ_H_