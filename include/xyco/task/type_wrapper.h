#ifndef XYCO_TASK_TYPE_WRAPPER_H_
#define XYCO_TASK_TYPE_WRAPPER_H_

namespace xyco::task {
template <typename T>
class TypeWrapper {
 public:
  T inner_;
};

template <>
class TypeWrapper<void> {};
}  // namespace xyco::task

#endif  // XYCO_TASK_TYPE_WRAPPER_H_
