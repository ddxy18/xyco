#include "xyco/time/clock.h"

std::variant<std::chrono::system_clock, std::chrono::steady_clock,
             xyco::time::FrozenClock>
    xyco::time::Clock::clock_ = std::chrono::system_clock();

xyco::time::FrozenClock::time_point xyco::time::FrozenClock::time_point_ =
    xyco::time::FrozenClock::time_point::clock::now();
