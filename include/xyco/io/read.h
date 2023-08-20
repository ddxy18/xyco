#ifndef XYCO_IO_READ_H_
#define XYCO_IO_READ_H_

#include <concepts>
#include <span>

#include "xyco/io/utils.h"
#include "xyco/runtime/future.h"
#include "xyco/utils/error.h"

namespace xyco::io {
template <typename Reader, typename Iterator>
concept Readable = requires(Reader reader, Iterator begin, Iterator end) {
  {
    reader.read(begin, end)
  } -> std::same_as<runtime::Future<utils::Result<uintptr_t>>>;
};

template <typename Reader, typename Buffer>
concept BufferReadable = requires(Reader reader, Buffer buffer) {
  {
    reader.fill_buffer()
  } -> std::same_as<runtime::Future<utils::Result<
      std::pair<decltype(std::begin(buffer)), decltype(std::begin(buffer))>>>>;
  { reader.consume(0) } -> std::same_as<void>;
};

class ReadExt {
 public:
  template <typename Reader, typename B>
  // NOLINTNEXTLINE(cppcoreguidelines-avoid-reference-coroutine-parameters)
  static auto read(Reader &reader, B &buffer)
      -> runtime::Future<utils::Result<uintptr_t>>
    requires(Readable<Reader, decltype(std::begin(buffer))> && Buffer<B>)
  {
    co_return co_await reader.read(std::begin(buffer), std::end(buffer));
  }

  template <typename Reader, typename V, std::size_t N>
  // NOLINTNEXTLINE(cppcoreguidelines-avoid-c-arrays,modernize-avoid-c-arrays,cppcoreguidelines-avoid-reference-coroutine-parameters)
  static auto read(Reader &reader, V (&buffer)[N])
      -> runtime::Future<utils::Result<uintptr_t>>
    requires(Readable<Reader, decltype(std::begin(buffer))> &&
             Buffer<decltype(buffer)>)
  {
    auto span = std::span(buffer);
    co_return co_await read(reader, span);
  }
};

class BufferReadExt {
 public:
  template <typename Reader, typename B>
  // NOLINTNEXTLINE(cppcoreguidelines-avoid-reference-coroutine-parameters)
  static auto read_to_end(Reader &reader) -> runtime::Future<utils::Result<B>>
    requires(BufferReadable<Reader, B> && DynamicBuffer<B>)
  {
    B content;

    while (true) {
      auto [begin, end] = *co_await reader.fill_buffer();
      if (begin == end) {
        co_return content;
      }
      auto prev_size = std::size(content);
      content.resize(prev_size + std::distance(begin, end));
      std::copy(begin, end, std::begin(content) + prev_size);
      reader.consume(std::distance(begin, end));
    }
  }

  template <typename Reader, typename B>
  // NOLINTNEXTLINE(cppcoreguidelines-avoid-reference-coroutine-parameters)
  static auto read_until(Reader &reader, char character)
      -> runtime::Future<utils::Result<B>>
    requires(BufferReadable<Reader, B> && DynamicBuffer<B>)
  {
    B content;

    while (true) {
      auto [begin, end] = *co_await reader.fill_buffer();
      if (begin == end) {
        co_return content;
      }
      auto pos = std::find(begin, end, character);
      auto prev_size = std::size(content);
      content.resize(prev_size + std::distance(begin, pos + 1));
      std::copy(begin, pos + 1, std::begin(content) + prev_size);
      reader.consume(std::distance(begin, pos + 1));
      if (pos != end) {
        co_return content;
      }
    }
  }

  template <typename Reader, typename B>
  // NOLINTNEXTLINE(cppcoreguidelines-avoid-reference-coroutine-parameters)
  static auto read_line(Reader &reader) -> runtime::Future<utils::Result<B>>
    requires(BufferReadable<Reader, B> && DynamicBuffer<B>)
  {
    co_return co_await read_until<Reader, B>(reader, '\n');
  }
};
}  // namespace xyco::io

#endif  // XYCO_IO_READ_H_