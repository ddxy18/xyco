#include "registry.h"

auto runtime::Event::readable() const -> bool {
  return state_ == State::Readable || state_ == State::All;
}

auto runtime::Event::writeable() const -> bool {
  return state_ == State::Writable || state_ == State::All;
}

auto runtime::Event::clear_readable() -> void {
  if (state_ == State::Readable) {
    state_ = State::Pending;
  }
  if (state_ == State::All) {
    state_ = State::Writable;
  }
}

auto runtime::Event::clear_writeable() -> void {
  if (state_ == State::Writable) {
    state_ = State::Pending;
  }
  if (state_ == State::All) {
    state_ = State::Readable;
  }
}