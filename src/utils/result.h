#ifndef XYWEBSERVER_UTILS_RESULT_H_
#define XYWEBSERVER_UTILS_RESULT_H_

#include <variant>

#include "panic.h"
#include "spdlog/fmt/bundled/format.h"
#include "utils/logger.h"

template <typename T, typename E>
class Result;

template <typename T>
concept Printable = requires(T val) {
  fmt::print(fmt::runtime("{}"), val);
};

template <typename T, typename E>
auto Ok(T inner) -> Result<T, E>
requires(!std::is_same_v<T, void>);

template <typename E>
auto Ok() -> Result<void, E>;

template <typename T, typename E>
auto Err(E inner) -> Result<T, E>
requires(!std::is_same_v<E, void>);

template <typename T>
auto Err() -> Result<T, void>;

template <typename T, typename E>
class Result {
  friend auto Ok<T, E>(T inner) -> Result<T, E>
  requires(!std::is_same_v<T, void>);
  friend auto Err<T, E>(E inner) -> Result<T, E>
  requires(!std::is_same_v<E, void>);

 public:
  Result() = default;

  auto unwrap() -> T requires Printable<E> {
    if (inner_.index() == 1) {
      TRACE("unwrap err:{}\n", std::get<1>(inner_));
      panic();
    }
    return std::move(std::get<0>(inner_));
  }

  auto unwrap_or(T default_val) -> T {
    if (inner_.index() == 0) {
      return std::get<0>(inner_);
    }
    return default_val;
  }

  auto unwrap_err() -> E {
    if (inner_.index() == 0) {
      panic();
    }
    return std::get<1>(inner_);
    // return std::get<1>(inner_);  // unreachable
  }

  template <typename MapT>
  [[nodiscard]] auto map(const std::function<MapT(T)>& f) -> Result<MapT, E> {
    if constexpr (std::is_same_v<MapT, void>) {
      if (inner_.index() == 0) {
        return Ok<E>();
      }
      return Err<MapT, E>(std::get<1>(inner_));
    } else {
      if (inner_.index() == 0) {
        return Ok<MapT, E>(f(std::get<0>(inner_)));
      }
      return Err<MapT, E>(std::get<1>(inner_));
    }
  }

  template <typename MapE>
  [[nodiscard]] auto map_err(const std::function<MapE(E)>& f)
      -> Result<T, MapE> {
    if constexpr (std::is_same_v<MapE, void>) {
      if (inner_.index() == 1) {
        return Err<T>();
      }
      return Ok<T, MapE>(std::get<0>(inner_));
    } else {
      if (inner_.index() == 1) {
        return Err<T, MapE>(f(std::get<1>(inner_)));
      }
      return Ok<T, MapE>(std::get<0>(inner_));
    }
  }

  auto is_ok() -> bool { return inner_.index() == 0; }

  auto is_err() -> bool { return !is_ok(); }

 private:
  explicit Result(std::variant<T, E, bool>&& inner)
      : inner_(std::move(inner)) {}

  std::variant<T, E, bool> inner_{std::in_place_index<2>, true};
};

template <>
class Result<void, void> {
  friend auto Ok<void>() -> Result<void, void>;
  friend auto Err<void>() -> Result<void, void>;

 public:
  Result() = default;

  auto unwrap() -> void {
    if (inner_.index() == 1) {
      TRACE("unwrap err:{void}\n");
      panic();
    }
  }

  auto unwrap_or() -> void {}

  auto unwrap_err() -> void {
    if (inner_.index() == 0) {
      panic();
    }
  }

  template <typename MapT>
  [[nodiscard]] auto map(const std::function<MapT(void)>& f)
      -> Result<MapT, void>
  requires(!std::is_void<MapT>::value) {
    if (inner_.index() == 0) {
      return Ok<MapT, void>(f());
    }
    return Err<MapT>();
  }

  template <typename MapE>
  [[nodiscard]] auto map_err(const std::function<MapE(void)>& f)
      -> Result<void, MapE>
  requires(!std::is_void<MapE>::value) {
    if (inner_.index() == 1) {
      return Err<void, MapE>(f());
    }
    return Ok<MapE>();
  }

  auto is_ok() -> bool { return inner_.index() == 0; }

  auto is_err() -> bool { return !is_ok(); }

 private:
  explicit Result(std::variant<bool, bool>&& inner) : inner_(inner) {}

  std::variant<bool, bool> inner_;
};

template <typename T>
requires(!std::is_same_v<T, void>) class Result<T, void> {
  friend auto Ok<T, void>(T inner) -> Result<T, void>
  requires(!std::is_same_v<T, void>);
  friend auto Err<T>() -> Result<T, void>;

 public:
  Result() = default;

  auto unwrap() -> T {
    if (inner_.index() == 1) {
      TRACE("unwrap err:{void}\n");
      panic();
    }
    return std::get<0>(inner_);
  }

  auto unwrap_or(T default_val) -> T {
    return inner_.index() == 0 ? std::get<0>(inner_) : default_val;
  }

  auto unwrap_err() -> void {
    if (inner_.index() == 0) {
      panic();
    }
  }

  template <typename MapT>
  [[nodiscard]] auto map(const std::function<MapT(T)>& f)
      -> Result<MapT, void> {
    if constexpr (std::is_same_v<MapT, void>) {
      if (inner_.index() == 0) {
        return Ok<void>();
      }
      return Err<void>();
    } else {
      if (inner_.index() == 0) {
        return Ok<MapT, void>(f(std::get<0>(inner_)));
      }
      return Err<MapT>();
    }
  }

  template <typename MapE>
  [[nodiscard]] auto map_err(const std::function<MapE(void)>& f)
      -> Result<T, MapE>
  requires(!std::is_void<MapE>::value) {
    if (inner_.index() == 1) {
      return Err<T, MapE>(f());
    }
    return Ok<T, MapE>(std::get<0>(inner_));
  }

  auto is_ok() -> bool { return inner_.index() == 0; }

  auto is_err() -> bool { return !is_ok(); }

 private:
  explicit Result(std::variant<T, bool>&& ok) : inner_(std::move(ok)) {}

  std::variant<T, bool> inner_{std::in_place_index<1>, true};
};

template <typename E>
class Result<void, E> {
  friend auto Ok<E>() -> Result<void, E>;
  friend auto Err<void, E>(E inner) -> Result<void, E>
  requires(!std::is_same_v<E, void>);

 public:
  Result() = default;

  auto unwrap() -> void {
    if (inner_.index() == 1) {
      TRACE("unwrap err:{}\n", std::get<1>(inner_));
      panic();
    }
  }

  auto unwrap_or() -> void {}

  auto unwrap_err() -> E {
    if (inner_.index() == 0) {
      panic();
    }
    return std::get<1>(inner_);
  }

  template <typename MapT>
  [[nodiscard]] auto map(const std::function<MapT(void)>& f) -> Result<MapT, E>
  requires(!std::is_void<MapT>::value) {
    if (inner_.index() == 1) {
      return Err<MapT, E>(std::get<1>(inner_));
    }
    return Ok<MapT, E>(f());
  }

  template <typename MapE>
  [[nodiscard]] auto map_err(const std::function<MapE(E)>& f)
      -> Result<void, MapE> {
    if constexpr (std::is_same_v<MapE, void>) {
      if (inner_.index() == 1) {
        return Err<void>();
      }
      return Ok<void>();
    } else {
      if (inner_.index() == 1) {
        return Err<void, MapE>(f(std::get<1>(inner_)));
      }
      return Ok<MapE>();
    }
  }

  auto is_ok() -> bool { return inner_.index() == 0; }

  auto is_err() -> bool { return !is_ok(); }

 private:
  explicit Result(std::variant<bool, E> inner) : inner_(std::move(inner)) {}

  std::variant<bool, E> inner_;
};

template <typename T, typename E>
auto Ok(T inner) -> Result<T, E>
requires(!std::is_same_v<T, void>) {
  if constexpr (std::is_same_v<E, void>) {
    return Result<T, E>(
        std::variant<T, bool>(std::in_place_index<0>, std::move(inner)));
  } else {
    return Result<T, E>{
        std::variant<T, E, bool>(std::in_place_index<0>, std::move(inner))};
  }
}

template <typename E>
auto Ok() -> Result<void, E> {
  if constexpr (std::is_same_v<E, void>) {
    return Result<void, E>(
        std::variant<bool, bool>{std::in_place_index<0>, true});
  } else {
    return Result<void, E>(std::variant<bool, E>{std::in_place_index<0>, true});
  }
}

template <typename T, typename E>
auto Err(E inner) -> Result<T, E>
requires(!std::is_same_v<E, void>) {
  if constexpr (std::is_same_v<T, void>) {
    return Result<T, E>(
        std::variant<bool, E>{std::in_place_index<1>, std::move(inner)});

  } else {
    return Result<T, E>{std::move(
        std::variant<T, E, bool>{std::in_place_index<1>, std::move(inner)})};
  }
}

template <typename T>
auto Err() -> Result<T, void> {
  if constexpr (std::is_same_v<T, void>) {
    return Result<T, void>(
        std::variant<bool, bool>{std::in_place_index<1>, true});
  } else {
    return Result<T, void>(std::variant<T, bool>{std::in_place_index<1>, true});
  }
}

// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define TRY(result)          \
  ({                         \
    if ((result).is_err()) { \
      return (result);       \
    }                        \
    (result).unwrap();       \
  })

// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define ASYNC_TRY(result)    \
  ({                         \
    if ((result).is_err()) { \
      co_return(result);     \
    }                        \
    (result).unwrap();       \
  })

#endif  // XYWEBSERVER_UTILS_RESULT_H_