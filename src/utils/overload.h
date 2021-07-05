#ifndef XYWEBSERVER_UTILS_OVERLOAD_H_
#define XYWEBSERVER_UTILS_OVERLOAD_H_

template <class... Ts>
struct overload : Ts... {
  using Ts::operator()...;
};
template <class... Ts>
overload(Ts...) -> overload<Ts...>;

#endif  // XYWEBSERVER_UTILS_OVERLOAD_H_