#include "wheel.h"

xyco::time::Level::Level() : current_it_(events_.begin()) {}

auto xyco::time::Wheel::insert_event(runtime::Event &event) -> void {
  auto expire_time_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
      std::get<runtime::TimeExtra>(event.extra_).expire_time_ - now_);

  auto level = 0;
  auto tmp_expire_time_ms = expire_time_ms;
  while ((tmp_expire_time_ms /= Level::slot_num_).count() > 0) {
    level++;
  }

  auto *slot_it = levels_.at(level).current_it_ + 1;
  if (slot_it == levels_.at(level).events_.end()) {
    slot_it = levels_.at(level).events_.begin();
  }
  if (level == 0) {
    auto steps = expire_time_ms.count() / interval_ms_;
    slot_it = levels_.at(0).current_it_;
    if (std::distance(levels_.at(0).current_it_, levels_.at(0).events_.end()) <=
        steps) {
      std::advance(slot_it, steps - Level::slot_num_);
    } else {
      std::advance(slot_it, steps);
    }
  }
  slot_it->push_back(event);
}

auto xyco::time::Wheel::expire(runtime::Events &events) -> void {
  auto now = std::chrono::system_clock::now();
  while (now > now_) {
    events.insert(events.end(), levels_[0].current_it_->begin(),
                  levels_[0].current_it_->end());
    levels_[0].current_it_->clear();
    now_ += std::chrono::milliseconds(interval_ms_);
    levels_[0].current_it_++;
    if (levels_[0].current_it_ == levels_[0].events_.end()) {
      reinsert_level(1);
      levels_[0].current_it_ = levels_[0].events_.begin();
    }
  }
}

xyco::time::Wheel::Wheel()
    : now_(std::chrono::duration_cast<std::chrono::milliseconds>(
          std::chrono::system_clock::now().time_since_epoch())) {}

auto xyco::time::Wheel::reinsert_level(int level) -> void {
  if (level == level_num_ - 1) {
    return;
  }
  if (levels_.at(level).current_it_ == levels_.at(level).events_.end()) {
    reinsert_level(level + 1);
    levels_.at(level).current_it_ = levels_.at(level).events_.begin();
  }
  for (auto it = levels_.at(level).current_it_->begin();
       it < levels_.at(level).current_it_->end(); it++) {
    insert_event(*it);
  }
  levels_.at(level).current_it_->clear();
  levels_.at(level).current_it_++;
}