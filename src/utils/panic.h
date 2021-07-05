#ifndef XYWEBSERVER_UTILS_PANIC_H_
#define XYWEBSERVER_UTILS_PANIC_H_

#include <exception>
#include <stdexcept>

// terminate current thread
auto panic() -> void;

#endif  // XYWEBSERVER_UTILS_PANIC_H_