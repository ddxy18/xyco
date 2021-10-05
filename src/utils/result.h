#ifndef XYWEBSERVER_UTILS_RESULT_H_
#define XYWEBSERVER_UTILS_RESULT_H_

#include <type_traits>
#include <utility>
#include <variant>

#include "panic.h"
#include "spdlog/fmt/fmt.h"
#include "utils/logger.h"

template <typename T, typename E>
class Result;

template <typename T>
class Ok {
  template <typename T1, typename E1>
  friend class Result;

 private:
  Ok(T&& inner) : inner_(std::forward<T>(inner)) {}

  T inner_;
};

template <>
class Ok<void> {};

template <typename E>
class Err {
  template <typename T1, typename E1>
  friend class Result;

 private:
  Err(E&& inner) : inner_(std::forward<E>(inner)) {}

  E inner_;
};

template <>
class Err<void> {};

template <typename T>
concept Printable = requires(T val) {
  fmt::print(fmt::runtime("{}"), val);
};

template <typename T, typename E>
class Result {
 public:
  template <typename... Args>
  static auto ok(Args&&... inner) -> Result<T, E> {
    if constexpr (std::is_same_v<T, void>) {
      return Result<T, E>(
          std::variant<Ok<T>, Err<E>, bool>(std::in_place_index<0>, Ok<T>()));
    } else {
      return Result<T, E>(std::variant<Ok<T>, Err<E>, bool>(
          std::in_place_index<0>, Ok<T>(T(std::forward<Args>(inner)...))));
    }
  }

  template <typename... Args>
  static auto err(Args&&... inner) -> Result<T, E> {
    if constexpr (std::is_same_v<E, void>) {
      return Result<T, E>(
          std::variant<Ok<T>, Err<E>, bool>(std::in_place_index<1>, Err<E>()));
    } else {
      return Result<T, E>(std::variant<Ok<T>, Err<E>, bool>(
          std::in_place_index<1>, Err<E>(E(std::forward<Args>(inner)...))));
    }
  }

  auto unwrap() -> T requires(Printable<E> || std::is_void_v<E>) {
    if (inner_.index() == 1) {
      if constexpr (std::is_same_v<E, void>) {
        TRACE("unwrap err:{E=void}\n");
      } else {
        TRACE("unwrap err:{}\n", std::get<1>(inner_).inner_);
      }
      panic();
    }
    if constexpr (!std::is_same_v<T, void>) {
      return std::move(std::get<0>(inner_).inner_);
    }
  }

  auto unwrap_err() -> E {
    if (inner_.index() == 0) {
      panic();
    }
    if constexpr (!std::is_same_v<E, void>) {
      return std::get<1>(inner_).inner_;
    }
  }

  template <typename Fn>
  auto unwrap_or_else(const Fn& f)
      -> T requires std::is_invocable_v<Fn, E> &&(!std::is_same_v<E, void>) {
    if (inner_.index() == 1) {
      return f(std::get<1>(inner_).inner_);
    }
    if constexpr (!std::is_same_v<T, void>) {
      return std::get<0>(inner_).inner_;
    }
  }

  template <typename Fn>
  auto unwrap_or_else(const Fn& f)
      -> T requires std::is_invocable_v<Fn> && std::is_same_v<E, void> {
    if (inner_.index() == 1) {
      return f();
    }
    if constexpr (!std::is_same_v<T, void>) {
      return std::get<0>(inner_).inner_;
    }
  }

  template <typename Fn>
  [[nodiscard]] auto map(const Fn& f) requires std::is_invocable_v<Fn, T> &&
      (!std::is_same_v<T, void>) {
    using MapT = decltype(f(std::get<0>(
        std::variant<T, Err<E>, bool>(std::in_place_index<2>, true))));

    if (inner_.index() == 0) {
      if constexpr (std::is_same_v<MapT, void>) {
        f(std::get<0>(inner_).inner_);
        return Result<void, E>::ok();
      } else {
        return Result<MapT, E>::ok(f(std::get<0>(inner_).inner_));
      }
    }
    if constexpr (!std::is_same_v<E, void>) {
      return Result<MapT, E>::err(std::get<1>(inner_).inner_);
    } else {
      return Result<MapT, void>::err();
    }
  }

  template <typename Fn>
  [[nodiscard]] auto map(const Fn& f) -> Result<decltype(f()), E>
  requires std::is_invocable_v<Fn> && std::is_same_v<T, void> {
    using MapT = decltype(f());

    if (inner_.index() == 0) {
      if constexpr (std::is_same_v<MapT, void>) {
        f();
        return Result<void, E>::ok();
      } else {
        return Result<MapT, E>::ok(f());
      }
    }
    if constexpr (!std::is_same_v<E, void>) {
      return Result<MapT, E>::err(std::get<1>(inner_).inner_);
    } else {
      return Result<MapT, void>::err();
    }
  }

  template <typename Fn>
  [[nodiscard]] auto map_err(const Fn& f) requires std::is_invocable_v<Fn, E> &&
      (!std::is_same_v<E, void>) {
    using MapE = decltype(f(std::get<1>(
        std::variant<Ok<T>, E, bool>(std::in_place_index<2>, true))));

    if (inner_.index() == 1) {
      if constexpr (std::is_same_v<MapE, void>) {
        f(std::get<1>(inner_).inner_);
        return Result<T, void>::err();
      } else {
        return Result<T, MapE>::err(f(std::get<1>(inner_).inner_));
      }
    }
    if constexpr (std::is_same_v<T, void>) {
      return Result<void, MapE>::ok();
    } else {
      return Result<T, MapE>::ok(std::get<0>(inner_).inner_);
    }
  }

  template <typename Fn>
  [[nodiscard]] auto map_err(const Fn& f) -> Result<T, decltype(f())>
  requires std::is_invocable_v<Fn> && std::is_same_v<E, void> {
    using MapE = decltype(f());

    if (inner_.index() == 1) {
      if constexpr (std::is_same_v<MapE, void>) {
        f();
        return Result<T, void>::err();
      } else {
        return Result<T, MapE>::err(f());
      }
    }
    if constexpr (std::is_same_v<T, void>) {
      return Result<void, MapE>::ok();
    } else {
      return Result<T, MapE>::ok(std::get<0>(inner_).inner_);
    }
  }

  auto is_ok() -> bool { return inner_.index() == 0; }

  auto is_err() -> bool { return !is_ok(); }

  Result() = default;

 private:
  explicit Result(std::variant<Ok<T>, Err<E>, bool>&& inner)
      : inner_(std::move(inner)) {}

  std::variant<Ok<T>, Err<E>, bool> inner_{std::in_place_index<2>, true};
};

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