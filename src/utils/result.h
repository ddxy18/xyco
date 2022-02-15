#ifndef XYCO_UTILS_RESULT_H_
#define XYCO_UTILS_RESULT_H_

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
  using Inner = std::variant<std::monostate, Ok<T>, Err<E>>;

 public:
  template <typename... Args>
  static auto ok(Args&&... inner) -> Result<T, E> {
    if constexpr (std::is_same_v<T, void>) {
      return Result<T, E>(Inner(Ok<T>()));
    } else {
      return Result<T, E>(Inner(Ok<T>(T(std::forward<Args>(inner)...))));
    }
  }

  template <typename... Args>
  static auto err(Args&&... inner) -> Result<T, E> {
    if constexpr (std::is_same_v<E, void>) {
      return Result<T, E>(Inner(Err<E>()));
    } else {
      return Result<T, E>(Inner(Err<E>(E(std::forward<Args>(inner)...))));
    }
  }

  auto unwrap() -> T requires(Printable<E> || std::is_void_v<E>) {
    if (inner_.index() == 2) {
      if constexpr (std::is_same_v<E, void>) {
        ERROR("unwrap err:{E=void}");
      } else {
        ERROR("unwrap err:{}", std::get<Err<E>>(inner_).inner_);
      }
      panic();
    }
    if constexpr (!std::is_same_v<T, void>) {
      return std::move(std::get<Ok<T>>(inner_).inner_);
    }
  }

  auto unwrap_err() -> E requires(Printable<T> || std::is_void_v<T>) {
    if (inner_.index() == 1) {
      if constexpr (std::is_same_v<T, void>) {
        ERROR("unwrap_err err:{T=void}");
      } else {
        ERROR("unwrap_err err:{}", std::get<Ok<T>>(inner_).inner_);
      }
      panic();
    }
    if constexpr (!std::is_same_v<E, void>) {
      return std::get<Err<E>>(inner_).inner_;
    }
  }

  template <typename Fn>
  auto unwrap_or_else(const Fn& f)
      -> T requires std::is_invocable_v<Fn, E> &&(!std::is_same_v<E, void>) {
    if (inner_.index() == 2) {
      return f(std::get<Err<E>>(inner_).inner_);
    }
    if constexpr (!std::is_same_v<T, void>) {
      return std::get<Ok<T>>(inner_).inner_;
    }
  }

  template <typename Fn>
  auto unwrap_or_else(const Fn& f)
      -> T requires std::is_invocable_v<Fn> && std::is_same_v<E, void> {
    if (inner_.index() == 2) {
      return f();
    }
    if constexpr (!std::is_same_v<T, void>) {
      return std::get<Ok<T>>(inner_).inner_;
    }
  }

  template <typename Fn>
  [[nodiscard]] auto map(const Fn& f) requires std::is_invocable_v<Fn, T> &&
      (!std::is_same_v<T, void>) {
    using MapT =
        decltype(f(std::get<T>(std::variant<std::monostate, T, Err<E>>())));

    if (inner_.index() == 1) {
      if constexpr (std::is_same_v<MapT, void>) {
        f(std::get<Ok<T>>(inner_).inner_);
        return Result<void, E>::ok();
      } else {
        return Result<MapT, E>::ok(f(std::get<Ok<T>>(inner_).inner_));
      }
    }
    if constexpr (!std::is_same_v<E, void>) {
      return Result<MapT, E>::err(std::get<Err<E>>(inner_).inner_);
    } else {
      return Result<MapT, void>::err();
    }
  }

  template <typename Fn>
  [[nodiscard]] auto map(const Fn& f) -> Result<decltype(f()), E>
  requires std::is_invocable_v<Fn> && std::is_same_v<T, void> {
    using MapT = decltype(f());

    if (inner_.index() == 1) {
      if constexpr (std::is_same_v<MapT, void>) {
        f();
        return Result<void, E>::ok();
      } else {
        return Result<MapT, E>::ok(f());
      }
    }
    if constexpr (!std::is_same_v<E, void>) {
      return Result<MapT, E>::err(std::get<Err<E>>(inner_).inner_);
    } else {
      return Result<MapT, void>::err();
    }
  }

  template <typename Fn>
  [[nodiscard]] auto map_err(const Fn& f) requires std::is_invocable_v<Fn, E> &&
      (!std::is_same_v<E, void>) {
    using MapE =
        decltype(f(std::get<E>(std::variant<std::monostate, Ok<T>, E>())));

    if (inner_.index() == 2) {
      if constexpr (std::is_same_v<MapE, void>) {
        f(std::get<Err<E>>(inner_).inner_);
        return Result<T, void>::err();
      } else {
        return Result<T, MapE>::err(f(std::get<Err<E>>(inner_).inner_));
      }
    }
    if constexpr (std::is_same_v<T, void>) {
      return Result<void, MapE>::ok();
    } else {
      return Result<T, MapE>::ok(std::get<Ok<T>>(inner_).inner_);
    }
  }

  template <typename Fn>
  [[nodiscard]] auto map_err(const Fn& f) -> Result<T, decltype(f())>
  requires std::is_invocable_v<Fn> && std::is_same_v<E, void> {
    using MapE = decltype(f());

    if (inner_.index() == 2) {
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
      return Result<T, MapE>::ok(std::get<Ok<T>>(inner_).inner_);
    }
  }

  auto is_ok() -> bool { return inner_.index() == 1; }

  auto is_err() -> bool { return !is_ok(); }

  template <typename CastT, typename CastE>
  operator Result<CastT, CastE>() {
    if (is_ok()) {
      if constexpr (std::is_same_v<CastT, void>) {
        return Result<CastT, CastE>::ok();
      } else {
        return Result<CastT, CastE>::ok(
            static_cast<CastT>(std::get<Ok<T>>(inner_).inner_));
      }
    }
    if constexpr (std::is_same_v<CastE, void>) {
      return Result<CastT, CastE>::err();
    } else {
      return Result<CastT, CastE>::err(
          static_cast<CastE>(std::get<Err<E>>(inner_).inner_));
    }
  }

 private:
  explicit Result(Inner&& inner) : inner_(std::move(inner)) {}

  Inner inner_;
};

// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define TRY(result)                        \
  ({                                       \
    auto result_ref = std::move((result)); \
    if (result_ref.is_err()) {             \
      return std::move(result_ref);        \
    }                                      \
    result_ref.unwrap();                   \
  })

// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define ASYNC_TRY(result)                  \
  ({                                       \
    auto result_ref = std::move((result)); \
    if (result_ref.is_err()) {             \
      co_return std::move(result_ref);     \
    }                                      \
    result_ref.unwrap();                   \
  })

#endif  // XYCO_UTILS_RESULT_H_