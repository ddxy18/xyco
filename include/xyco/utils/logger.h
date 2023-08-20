#ifndef XYCO_UTILS_LOGGER_H_
#define XYCO_UTILS_LOGGER_H_

#ifdef XYCO_ENABLE_LOG

#include <format>

#include "spdlog/spdlog.h"

#undef SPDLOG_ACTIVE_LEVEL
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define SPDLOG_ACTIVE_LEVEL SPDLOG_LEVEL_TRACE

#undef TRACE
#undef DEBUG
#undef INFO
#undef WARN
#undef ERROR

// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define TRACE(...)                                                        \
  SPDLOG_LOGGER_CALL(LoggerCtx::get_logger(), spdlog::level::trace, "{}", \
                     std::format(__VA_ARGS__))
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define DEBUG(...)                                                        \
  SPDLOG_LOGGER_CALL(LoggerCtx::get_logger(), spdlog::level::debug, "{}", \
                     std::format(__VA_ARGS__))
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define INFO(...)                                                        \
  SPDLOG_LOGGER_CALL(LoggerCtx::get_logger(), spdlog::level::info, "{}", \
                     std::format(__VA_ARGS__))
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define WARN(...)                                                        \
  SPDLOG_LOGGER_CALL(LoggerCtx::get_logger(), spdlog::level::warn, "{}", \
                     std::format(__VA_ARGS__))
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define ERROR(...)                                                      \
  SPDLOG_LOGGER_CALL(LoggerCtx::get_logger(), spdlog::level::err, "{}", \
                     std::format(__VA_ARGS__))

class LoggerCtx {
 public:
  static auto get_logger() -> std::shared_ptr<spdlog::logger>;
};

#else

template <typename... T>
// NOLINTNEXTLINE(cppcoreguidelines-missing-std-forward)
constexpr auto TRACE([[maybe_unused]] T&&... unused) {}

template <typename... T>
// NOLINTNEXTLINE(cppcoreguidelines-missing-std-forward)
constexpr auto DEBUG([[maybe_unused]] T&&... unused) {}

template <typename... T>
// NOLINTNEXTLINE(cppcoreguidelines-missing-std-forward)
constexpr auto INFO([[maybe_unused]] T&&... unused) {}

template <typename... T>
// NOLINTNEXTLINE(cppcoreguidelines-missing-std-forward)
constexpr auto WARN([[maybe_unused]] T&&... unused) {}

template <typename... T>
// NOLINTNEXTLINE(cppcoreguidelines-missing-std-forward)
constexpr auto ERROR([[maybe_unused]] T&&... unused) {}

#endif

#endif  // XYCO_UTILS_LOGGER_H_
