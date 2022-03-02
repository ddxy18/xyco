#define UNW_LOCAL_ONLY

#include "panic.h"

#include <unistd.h>

#include <array>
#include <sstream>
#include <vector>

#include "libunwind.h"
#include "logger.h"
#include "spdlog/formatter.h"

class Frame {
  friend auto unwind() -> std::vector<Frame>;
  friend struct fmt::formatter<Frame>;

 public:
  static constexpr int SYMBOL_SIZE = 256;

  Frame() : symbol_(SYMBOL_SIZE, 0) {}

 private:
  std::string symbol_;
  std::string line_;
  unw_word_t offset_{};
  unw_word_t ip_{};
};

auto addr2line(std::vector<unw_word_t> addresses) -> std::string {
  constexpr int PATH_SIZE = 256;

  std::array<int, 2> pipe{};
  ::pipe(pipe.data());
  auto pid = ::fork();
  if (pid == 0) {
    ::close(pipe[0]);
    ::dup2(pipe[1], STDOUT_FILENO);
    auto exe_path = std::string(PATH_SIZE, 0);
    auto rlin_size =
        ::readlink("/proc/self/exe", exe_path.data(), exe_path.size());
    exe_path.resize(rlin_size);
    auto cmd = fmt::format("/usr/bin/addr2line -e {} -Cf {:#x}", exe_path,
                           fmt::join(addresses, " "));
    std::system(cmd.data());
    ::close(STDOUT_FILENO);
    ::close(pipe[1]);
    std::quick_exit(0);
  } else if (pid > 0) {
    ::close(pipe[1]);

    auto demangle_symbol =
        std::string(Frame::SYMBOL_SIZE * addresses.size(), '0');
    decltype(::read(0, nullptr, 0)) nbytes = 1;
    decltype(::read(0, nullptr, 0)) total_bytes = 0;
    while (nbytes > 0) {
      nbytes = ::read(pipe[0], (demangle_symbol.begin() + total_bytes).base(),
                      demangle_symbol.size() - total_bytes);
      total_bytes += nbytes;
    }
    demangle_symbol.resize(total_bytes);
    return demangle_symbol;
  }
  return {};
}

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

  auto addresses = std::vector<unw_word_t>(frames.size());
  std::transform(frames.begin(), frames.end(), addresses.begin(),
                 [](auto frame) { return frame.ip_; });
  auto loc = addr2line(addresses);
  auto loc_stream = std::stringstream(loc);
  for (int i = 0; i < addresses.size(); i++) {
    std::string symbol;
    std::getline(loc_stream, symbol);
    if (symbol != "??") {
      frames[i].symbol_ = symbol;
    }
    std::getline(loc_stream, frames[i].line_);
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
    return format_to(ctx.out(), "0x{:0>16x} {} at {}+{:#x}", frame.ip_, symbol,
                     frame.line_, frame.offset_);
  }
};

auto panic() -> void {
  auto unwind_info = fmt::format("panic!\n{}", fmt::join(unwind(), "\n"));
  ERROR(unwind_info);
  throw std::runtime_error(unwind_info);
}