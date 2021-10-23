#ifndef XYCO_IO_BUFFER_READER_H_
#define XYCO_IO_BUFFER_READER_H_

#include "read.h"
#include "runtime/future.h"

namespace xyco::io {
const int DEFAULT_BUFFER_SIZE = 8 * 1024;  // 8 KB

template <typename Reader>
requires(Readable<Reader, std::vector<char>::iterator>) class BufferReader {
 public:
  template <typename Iterator>
  auto read(Iterator begin, Iterator end)
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

  auto fill_buffer() -> runtime::Future<io::IoResult<
      std::pair<std::vector<char>::iterator, std::vector<char>::iterator>>> {
    if (pos_ == cap_) {
      ASYNC_TRY((co_await ReadExt<Reader>::read(inner_reader_, buffer_))
                    .map([&](auto nbytes) {
                      cap_ = nbytes;
                      pos_ = 0;
                      return std::pair<std::vector<char>::iterator,
                                       std::vector<char>::iterator>(
                          buffer_.begin(), buffer_.begin() + cap_);
                    }));
    }
    co_return IoResult<
        std::pair<std::vector<char>::iterator, std::vector<char>::iterator>>::
        ok(buffer_.begin(), buffer_.begin() + cap_);
  }

  auto consume(uint16_t amt) -> void {
    pos_ = std::min(static_cast<decltype(pos_)>(pos_ + amt), cap_);
  }

  BufferReader(Reader&& reader)
      : inner_reader_(std::move(reader)),
        buffer_(DEFAULT_BUFFER_SIZE),
        pos_(0),
        cap_(0) {}

 private:
  Reader inner_reader_;

  std::vector<char> buffer_;
  uint16_t pos_;
  uint16_t cap_;
};
}  // namespace xyco::io

#endif  // XYCO_IO_BUFFER_READER_H_