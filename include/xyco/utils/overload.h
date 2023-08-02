#ifndef XYCO_UTILS_OVERLOAD_H_
#define XYCO_UTILS_OVERLOAD_H_

template <class... Ts>
struct overloaded : Ts... {
  using Ts::operator()...;
};
// explicit deduction guide (not needed as of C++20)
template <class... Ts>
overloaded(Ts...) -> overloaded<Ts...>;

#endif  // XYCO_UTILS_OVERLOAD_H_