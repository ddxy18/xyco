#ifndef XYCO_TIME_CLOCK_H
#define XYCO_TIME_CLOCK_H

#include <chrono>
#include <variant>

#include "utils/overload.h"

namespace xyco::time {
template <typename C>
concept Clockable = requires {
  { C::now() } -> std::same_as<typename C::time_point>;
};

class FrozenClock {
  using time_point = std::chrono::system_clock::time_point;

 public:
  static auto now() -> time_point { return time_point_; }

  template <typename Rep, typename Ratio>
  static auto advance(std::chrono::duration<Rep, Ratio> duration) -> void {
    time_point_ += duration;
  }

 private:
  static time_point time_point_;
};

class Clock {
 public:
  static auto now() -> std::chrono::system_clock::time_point {
    return std::visit(
        overloaded{
            [](std::chrono::system_clock clock) {
              return std::chrono::system_clock::now();
            },
            [](FrozenClock clock) { return xyco::time::FrozenClock::now(); },
            [](auto clock) {
              auto now = clock.now();
              return std::chrono::time_point_cast<
                  std::chrono::system_clock::duration>(
                  now - clock.now() + std::chrono::system_clock::now());
            }},
        clock_);
  }

  template <typename C>
  static auto init() -> void
    requires(Clockable<C>)
  {
    if constexpr (std::is_same_v<C, std::chrono::steady_clock>) {
      clock_ = std::chrono::steady_clock();
    } else if constexpr (std::is_same_v<C, FrozenClock>) {
      clock_ = FrozenClock();
    } else {
      clock_ = std::chrono::system_clock();
    }
  }

 private:
  static std::variant<std::chrono::system_clock, std::chrono::steady_clock,
                      FrozenClock>
      clock_;
};
}  // namespace xyco::time

#endif  // XYCO_TIME_CLOCK_H
