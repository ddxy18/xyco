#include "xyco/time/wheel.h"

#include "xyco/time/clock.h"
#include "xyco/time/driver.h"

xyco::time::Level::Level() : current_it_(events_.begin()) {}

auto xyco::time::Wheel::insert_event(std::shared_ptr<runtime::Event> event)
    -> void {
  auto total_steps =
      std::chrono::duration_cast<std::chrono::milliseconds>(
          dynamic_cast<TimeExtra *>(event->extra_.get())->expire_time_ - now_)
          .count();

  auto level = 0;
  auto steps = 0LL;
  while (total_steps > 0) {
    steps = total_steps % Level::slot_num_;
    auto another_cycle =
        steps >= std::distance(levels_.at(level).current_it_,
                               levels_.at(level).events_.end());
    total_steps =
        total_steps / Level::slot_num_ + static_cast<int>(another_cycle);
    level++;
  }
  level = std::max(--level, 0);

  auto *slot_it = levels_.at(level).current_it_;
  if (steps >= std::distance(slot_it, levels_.at(level).events_.end())) {
    steps -= Level::slot_num_;
  }
  std::advance(slot_it, steps);
  slot_it->push_back(std::move(event));
}

auto xyco::time::Wheel::expire(runtime::Events &events) -> void {
  auto now = Clock::now();
  if (now > now_) {
    auto steps =
        std::chrono::duration_cast<std::chrono::milliseconds>(now - now_)
            .count() /
        interval_ms_;
    now_ = now;
    walk(static_cast<int>(steps), events);
  }
}

xyco::time::Wheel::Wheel() : now_(Clock::now()) {}

auto xyco::time::Wheel::walk(int steps, runtime::Events &events) -> void {
  auto level = 0;
  auto level_steps = std::array<int, Level::slot_num_>();
  while (steps > 0) {
    level_steps.at(level) = steps % Level::slot_num_;
    auto another_cycle =
        level_steps.at(level) >= std::distance(levels_.at(level).current_it_,
                                               levels_.at(level).events_.end());
    steps = steps / Level::slot_num_ + static_cast<int>(another_cycle);
    level++;
  }
  level = std::max(--level, 0);

  for (auto i = 0; i < level; i++) {
    auto *slot_it = levels_.at(i).current_it_;
    for (auto j = 0; j < Level::slot_num_; j++) {
      events.insert(events.end(), slot_it->begin(), slot_it->end());
      slot_it->clear();
      std::advance(slot_it, 1);
      if (slot_it == levels_.at(i).events_.end()) {
        slot_it = levels_.at(i).events_.begin();
      }
    }
    auto tmp_steps = level_steps.at(i);
    if (tmp_steps >= std::distance(slot_it, levels_.at(i).events_.end())) {
      tmp_steps -= Level::slot_num_;
    }
    std::advance(slot_it, tmp_steps);
    levels_.at(i).current_it_ = slot_it;
  }

  for (auto i = 0; i < level_steps.at(level); i++) {
    auto *slot_it = levels_.at(level).current_it_;
    events.insert(events.end(), slot_it->begin(), slot_it->end());
    slot_it->clear();
    std::advance(slot_it, 1);
    if (slot_it == levels_.at(level).events_.end()) {
      slot_it = levels_.at(level).events_.begin();
    }
    levels_.at(level).current_it_ = slot_it;
  }

  // Move current and next slot events to lower level which helps to distinguish
  // the correct sequence of them.
  auto *slot_it = levels_.at(level).current_it_;
  auto reinsert_events = runtime::Events(slot_it->begin(), slot_it->end());
  slot_it->clear();
  for (auto it = reinsert_events.begin(); it < reinsert_events.end(); it++) {
    insert_event(*it);
  }
  std::advance(slot_it, 1);
  if (slot_it == levels_.at(level).events_.end()) {
    slot_it = levels_.at(level).events_.begin();
  }
  reinsert_events = runtime::Events(slot_it->begin(), slot_it->end());
  slot_it->clear();
  for (auto it = reinsert_events.begin(); it < reinsert_events.end(); it++) {
    insert_event(*it);
  }

  // Take all events at `levels_[0].current_it_` again since `insert_event` may
  // push some other events here.
  slot_it = levels_.at(0).current_it_;
  events.insert(events.end(), slot_it->begin(), slot_it->end());
  slot_it->clear();
}
