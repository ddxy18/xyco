#ifndef XYCO_TIME_WHEEL_H
#define XYCO_TIME_WHEEL_H

#include <array>
#include <chrono>
#include <vector>

#include "runtime/registry.h"

namespace xyco::time {
class Level {
  friend class Wheel;

 public:
  Level();

 private:
  constexpr static int slot_num_ = 60;

  std::array<runtime::Events, slot_num_> events_;

  decltype(events_.begin()) current_it_;
};

class Wheel {
 public:
  auto insert_event(std::weak_ptr<runtime::Event> event) -> void;

  auto expire(runtime::Events &events) -> void;

  Wheel();

 private:
  auto reinsert_level(int level) -> void;

  constexpr static int level_num_ = 5;

  constexpr static int interval_ms_ = 1;

  // up to 9 days
  std::array<Level, level_num_> levels_;

  std::chrono::time_point<std::chrono::system_clock> now_;
};
}  // namespace xyco::time

#endif  // XYCO_TIME_WHEEL_H