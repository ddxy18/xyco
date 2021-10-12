#include "registry.h"

auto reactor::Event::readable() const -> bool {
  return state_ == State::Readable || state_ == State::All;
}

auto reactor::Event::writeable() const -> bool {
  return state_ == State::Writable || state_ == State::All;
}

auto reactor::Event::clear_readable() -> void {
  if (state_ == State::Readable) {
    state_ = State::Pending;
  }
  if (state_ == State::All) {
    state_ = State::Writable;
  }
}

auto reactor::Event::clear_writeable() -> void {
  if (state_ == State::Writable) {
    state_ = State::Pending;
  }
  if (state_ == State::All) {
    state_ = State::Readable;
  }
}