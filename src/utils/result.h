#ifndef XYWEBSERVER_UTILS_RESULT_H_
#define XYWEBSERVER_UTILS_RESULT_H_

#include <concepts>
#include <functional>
#include <optional>

#include "fmt/core.h"
#include "panic.h"

template <typename T, typename E>
concept AllNonVoid = (!std::is_void<T>::value && !std::is_void<E>::value);

class Void {};

template <typename T, typename E>
requires AllNonVoid<T, E>
class Result;

template <typename T>
concept Printable = requires(T val) {
  fmt::print(fmt::runtime("{}"), val);
};

template <typename T, typename E>
auto Ok(T inner) -> Result<T, E>
requires AllNonVoid<T, E>;

template <typename T, typename E>
auto Ok() -> Result<T, E>
requires(std::is_same<T, Void>::value&& AllNonVoid<T, E>);

template <typename T, typename E>
auto Err(E inner) -> Result<T, E>
requires AllNonVoid<T, E>;

template <typename T, typename E>
auto Err() -> Result<T, E>
requires(std::is_same<E, Void>::value&& AllNonVoid<T, E>);

template <typename T, typename E>
requires AllNonVoid<T, E>
class Result {
  friend auto Ok<T, E>(T inner) -> Result<T, E>
  requires AllNonVoid<T, E>;
  friend auto Err<T, E>(E inner) -> Result<T, E>
  requires AllNonVoid<T, E>;

 public : Result() = default;
  auto unwrap() -> T requires Printable<T> {
    if (ok_.has_value()) {
      return std::move(*ok_);
    }
    fmt::print("unwrap err:{}\n", *err_);
    panic();
    return std::move(*ok_);  // unreachable
  }

  auto unwrap_or(T default_val) -> T {
    if (ok_.has_value()) {
      return std::move(*ok_);
    }
    return default_val;
  }

  auto unwrap_err() -> E {
    if (err_.has_value()) {
      return std::move(*err_);
    }
    panic();
    return std::move(*err_);  // unreachable
  }

  template <typename MapT>
  [[nodiscard]] auto map(const std::function<MapT(T)>& f) -> Result<MapT, E>
  requires(!std::is_void<MapT>::value) {
    if (ok_.has_value()) {
      return Ok<MapT, E>(f(ok_.value()));
    }
    return Err<MapT, E>(std::move(*err_));
  }

  template <typename MapE>
  [[nodiscard]] auto map_err(const std::function<MapE(E)>& f) -> Result<T, MapE>
  requires(!std::is_void<MapE>::value) {
    if (err_.has_value()) {
      return Err<T, MapE>(f(err_.value()));
    }
    return Ok<T, MapE>(std::move(*ok_));
  }

  auto is_ok() -> bool { return ok_.has_value(); }

  auto is_err() -> bool { return !is_ok(); }

 private:
  explicit Result(std::optional<T> ok, std::optional<E> err)
      : ok_(std::move(ok)), err_(std::move(err)) {}

  std::optional<T> ok_;
  std::optional<E> err_;
};

template <typename T, typename E>
auto Ok(T inner) -> Result<T, E>
requires AllNonVoid<T, E> { return Result<T, E>({std::move(inner)}, {}); }

template <typename T, typename E>
auto Ok() -> Result<T, E>
requires(std::is_same<T, Void>::value&& AllNonVoid<T, E>) {
  return Ok<T, E>(Void{});
}

template <typename T, typename E>
auto Err(E inner) -> Result<T, E>
requires AllNonVoid<T, E> { return Result<T, E>({}, {inner}); }

template <typename T, typename E>
auto Err() -> Result<T, E>
requires(std::is_same<E, Void>::value&& AllNonVoid<T, E>) {
  return Err<T, E>(Void{});
}

#define TRY(result)          \
  ({                         \
    if ((result).is_err()) { \
      return (result);       \
    }                        \
    (result).unwrap();       \
  })

#define ASYNC_TRY(result)    \
  ({                         \
    if ((result).is_err()) { \
      co_return(result);     \
    }                        \
    (result).unwrap();       \
  })

#endif  // XYWEBSERVER_UTILS_RESULT_H_