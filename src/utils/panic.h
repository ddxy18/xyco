#ifndef XYCO_UTILS_PANIC_H_
#define XYCO_UTILS_PANIC_H_

#include <string>

namespace xyco::utils {
// Terminates current thread
auto panic(std::string info = "") -> void;
}  // namespace xyco::utils

#endif  // XYCO_UTILS_PANIC_H_
