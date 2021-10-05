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
auto ok(T inner) -> Result<T, E>
requires(!std::is_same_v<T, void>);

template <typename E>
auto ok() -> Result<void, E>;

template <typename T, typename E>
auto err(E inner) -> Result<T, E>
requires(!std::is_same_v<E, void>);

template <typename T>
auto err() -> Result<T, void>;

template <typename T, typename E>
class Result {
  friend auto ok<T, E>(T inner) -> Result<T, E>
  requires(!std::is_same_v<T, void>);
  friend auto err<T, E>(E inner) -> Result<T, E>
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
  }

  template <typename Fn>
  [[nodiscard]] auto map(const Fn& f)
      -> Result<decltype(f(std::get<0>(std::variant<T, E, bool>{
                    std::in_place_index<2>, true}))),
                E>
  requires std::is_invocable_v<Fn, T> {
    using MapT = decltype(f(std::get<0>(this->inner_)));

    if constexpr (std::is_same_v<MapT, void>) {
      if (inner_.index() == 0) {
        return ok<E>();
      }
      return err<MapT, E>(std::get<1>(inner_));
    } else {
      if (inner_.index() == 0) {
        return ok<MapT, E>(f(std::get<0>(inner_)));
      }
      return err<MapT, E>(std::get<1>(inner_));
    }
  }

  template <typename Fn>
  [[nodiscard]] auto map_err(const Fn& f)
      -> Result<T, decltype(f(std::get<1>(std::variant<T, E, bool>{
                       std::in_place_index<2>, true})))>
  requires std::is_invocable_v<Fn, E> {
    using MapE = decltype(f(std::get<1>(this->inner_)));

    if constexpr (std::is_same_v<MapE, void>) {
      if (inner_.index() == 1) {
        return err<T>();
      }
      return ok<T, MapE>(std::get<0>(inner_));
    } else {
      if (inner_.index() == 1) {
        return err<T, MapE>(f(std::get<1>(inner_)));
      }
      return ok<T, MapE>(std::get<0>(inner_));
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
  friend auto ok<void>() -> Result<void, void>;
  friend auto err<void>() -> Result<void, void>;

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

  template <typename Fn>
  [[nodiscard]] auto map(const Fn& f) -> Result<decltype(f()), void>
  requires(!std::is_void<decltype(f())>::value && std::is_invocable_v<Fn>) {
    using MapT = decltype(f());

    if (inner_.index() == 0) {
      return ok<MapT, void>(f());
    }
    return err<MapT>();
  }

  template <typename Fn>
  [[nodiscard]] auto map_err(const Fn& f) -> Result<void, decltype(f())>
  requires(!std::is_void<decltype(f())>::value && std::is_invocable_v<Fn>) {
    using MapE = decltype(f());

    if (inner_.index() == 1) {
      return err<void, MapE>(f());
    }
    return ok<MapE>();
  }

  auto is_ok() -> bool { return inner_.index() == 0; }

  auto is_err() -> bool { return !is_ok(); }

 private:
  explicit Result(std::variant<bool, bool>&& inner) : inner_(inner) {}

  std::variant<bool, bool> inner_;
};

template <typename T>
requires(!std::is_same_v<T, void>) class Result<T, void> {
  friend auto ok<T, void>(T inner) -> Result<T, void>
  requires(!std::is_same_v<T, void>);
  friend auto err<T>() -> Result<T, void>;

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

  template <typename Fn>
  [[nodiscard]] auto map(const Fn& f)
      -> Result<decltype(f(std::get<0>(
                    std::variant<T, bool>(std::in_place_index<1>, true)))),
                void>
  requires(std::is_invocable_v<Fn, T>) {
    using MapT = decltype(f(std::get<0>(this->inner_)));

    if constexpr (std::is_same_v<MapT, void>) {
      if (inner_.index() == 0) {
        return ok<void>();
      }
      return err<void>();
    } else {
      if (inner_.index() == 0) {
        return ok<MapT, void>(f(std::get<0>(inner_)));
      }
      return err<MapT>();
    }
  }

  template <typename Fn>
  [[nodiscard]] auto map_err(const Fn& f) -> Result<T, decltype(f())>
  requires(!std::is_void<decltype(f())>::value && std::is_invocable_v<Fn>) {
    using MapE = decltype(f());

    if (inner_.index() == 1) {
      return err<T, MapE>(f());
    }
    return ok<T, MapE>(std::get<0>(inner_));
  }

  auto is_ok() -> bool { return inner_.index() == 0; }

  auto is_err() -> bool { return !is_ok(); }

 private:
  explicit Result(std::variant<T, bool>&& ok) : inner_(std::move(ok)) {}

  std::variant<T, bool> inner_{std::in_place_index<1>, true};
};

template <typename E>
class Result<void, E> {
  friend auto ok<E>() -> Result<void, E>;
  friend auto err<void, E>(E inner) -> Result<void, E>
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

  template <typename Fn>
  [[nodiscard]] auto map(const Fn& f) -> Result<decltype(f()), E>
  requires(!std::is_void<decltype(f())>::value && std::is_invocable_v<Fn>) {
    using MapT = decltype(f());

    if (inner_.index() == 1) {
      return err<MapT, E>(std::get<1>(inner_));
    }
    return ok<MapT, E>(f());
  }

  template <typename Fn>
  [[nodiscard]] auto map_err(const Fn& f)
      -> Result<void, decltype(f(std::get<1>(std::variant<bool, E>(
                          std::in_place_index<0>, true))))>
  requires std::is_invocable_v<Fn, E> {
    using MapE = decltype(f(std::get<1>(std::variant<bool, E>())));

    if constexpr (std::is_same_v<MapE, void>) {
      if (inner_.index() == 1) {
        return err<void>();
      }
      return ok<void>();
    } else {
      if (inner_.index() == 1) {
        return err<void, MapE>(f(std::get<1>(inner_)));
      }
      return ok<MapE>();
    }
  }

  auto is_ok() -> bool { return inner_.index() == 0; }

  auto is_err() -> bool { return !is_ok(); }

 private:
  explicit Result(std::variant<bool, E> inner) : inner_(std::move(inner)) {}

  std::variant<bool, E> inner_;
};

template <typename T, typename E>
auto ok(T inner) -> Result<T, E>
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
auto ok() -> Result<void, E> {
  if constexpr (std::is_same_v<E, void>) {
    return Result<void, E>(
        std::variant<bool, bool>{std::in_place_index<0>, true});
  } else {
    return Result<void, E>(std::variant<bool, E>{std::in_place_index<0>, true});
  }
}

template <typename T, typename E>
auto err(E inner) -> Result<T, E>
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
auto err() -> Result<T, void> {
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