#define UNW_LOCAL_ONLY

#include "panic.h"

#include <vector>

#include "libunwind.h"
#include "logger.h"
#include "spdlog/formatter.h"

class Frame {
  friend auto unwind() -> std::vector<Frame>;
  friend struct fmt::formatter<Frame>;

 public:
  Frame() : symbol_(SYMBOL_SIZE, '0') {}

 private:
  static constexpr int SYMBOL_SIZE = 256;

  std::string symbol_;
  unw_word_t offset_{};
  unw_word_t ip_{};
};

auto unwind() -> std::vector<Frame> {
  auto ctx = unw_context_t();
  unw_getcontext(&ctx);
  auto cursor = unw_cursor_t();
  unw_init_local(&cursor, &ctx);

  decltype(unwind()) frames;
  while (unw_step(&cursor) > 0) {
    auto frame = Frame();
    if (unw_get_proc_name(&cursor, frame.symbol_.data(), frame.symbol_.size(),
                          &frame.offset_) != 0) {
      frame.symbol_.clear();
    }
    unw_get_reg(&cursor, UNW_REG_IP, &frame.ip_);
    frames.push_back(frame);
  }

  return frames;
}

template <>
struct fmt::formatter<Frame> : public fmt::formatter<std::string> {
  template <typename FormatContext>
  auto format(const Frame& frame, FormatContext& ctx) const
      -> decltype(ctx.out()) {
    auto symbol = frame.symbol_;
    if (frame.symbol_.empty()) {
      symbol = "unkown name";
    }
    return format_to(ctx.out(), "{}+{:#x} 0x{:0>16x}", symbol, frame.offset_,
                     frame.ip_);
  }
};

auto panic() -> void {
  ERROR("panic!\n{}", fmt::join(unwind(), "\n"));
  throw std::runtime_error("panic");
}