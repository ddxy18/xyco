#ifndef XYCO_IO_BUFFER_READER_H_
#define XYCO_IO_BUFFER_READER_H_

#include "io/utils.h"
#include "read.h"

namespace xyco::io {
const int DEFAULT_BUFFER_SIZE = 8 * 1024;  // 8 KB

template <typename Reader, typename B>
requires(
    Readable<Reader, typename B::iterator>&& Buffer<B>) class BufferReader {
 public:
  auto read(typename B::iterator begin, typename B::iterator end)
      -> runtime::Future<io::IoResult<uintptr_t>> {
    auto len = std::distance(begin, end);
    if (len <= cap_ - pos_) {
      std::copy(buffer_.begin(), buffer_.begin() + len, begin);
      consume(len);
      co_return IoResult<uintptr_t>::ok(len);
    }
    ASYNC_TRY((co_await fill_buffer()).map([](auto pair) { return 0; }));
    len = std::min(len, static_cast<decltype(len)>(cap_ - pos_));
    std::copy(buffer_.begin(), buffer_.begin() + len, begin);
    consume(len);
    co_return IoResult<uintptr_t>::ok(len);
  }

  auto fill_buffer() -> runtime::Future<
      io::IoResult<std::pair<typename B::iterator, typename B::iterator>>> {
    if (pos_ == cap_) {
      ASYNC_TRY(
          (co_await ReadExt::read(inner_reader_, buffer_))
              .map([&](auto nbytes) {
                cap_ = nbytes;
                pos_ = 0;
                return std::pair<typename B::iterator, typename B::iterator>(
                    buffer_.begin(), buffer_.begin() + cap_);
              }));
    }
    co_return IoResult<std::pair<typename B::iterator, typename B::iterator>>::
        ok(buffer_.begin(), buffer_.begin() + cap_);
  }

  auto consume(uint16_t amt) -> void {
    pos_ = std::min(static_cast<decltype(pos_)>(pos_ + amt), cap_);
  }

  BufferReader(Reader& reader)
      : inner_reader_(reader),
        buffer_(DEFAULT_BUFFER_SIZE, 0),
        pos_(0),
        cap_(0) {}

 private:
  Reader& inner_reader_;

  B buffer_;
  typename B::size_type pos_;
  typename B::size_type cap_;
};
}  // namespace xyco::io

#endif  // XYCO_IO_BUFFER_READER_H_