#ifndef XYCO_RUNTIME_UTILS_TYPE_WRAPPER_H
#define XYCO_RUNTIME_UTILS_TYPE_WRAPPER_H

namespace xyco::runtime {
template <typename T>
class TypeWrapper {
 public:
  T inner_;
};

template <>
class TypeWrapper<void> {};
}  // namespace xyco::runtime

#endif  // XYCO_RUNTIME_UTILS_TYPE_WRAPPER_H