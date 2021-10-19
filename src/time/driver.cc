#include "driver.h"

#include "io/utils.h"

auto xyco::time::TimeRegistry::Register(runtime::Event &ev,
                                        runtime::Interest interest)
    -> io::IoResult<void> {
  wheel_.insert_event(ev);

  return io::IoResult<void>::ok();
}

// TODO(dongxiaoyu): support update expire time and cancel event
auto xyco::time::TimeRegistry::reregister(runtime::Event &ev,
                                          runtime::Interest interest)
    -> io::IoResult<void> {
  return io::IoResult<void>::ok();
}

auto xyco::time::TimeRegistry::deregister(runtime::Event &ev,
                                          runtime::Interest interest)
    -> io::IoResult<void> {
  return io::IoResult<void>::ok();
}

auto xyco::time::TimeRegistry::select(runtime::Events &events, int timeout)
    -> io::IoResult<void> {
  wheel_.expire(events);

  return io::IoResult<void>::ok();
}