#ifndef XYCO_UTILS_LOGGER_H_
#define XYCO_UTILS_LOGGER_H_

#ifdef XYCO_ENABLE_LOG

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
#define TRACE(...) \
  SPDLOG_LOGGER_CALL(LoggerCtx::get_logger(), spdlog::level::trace, __VA_ARGS__)
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define DEBUG(...) \
  SPDLOG_LOGGER_CALL(LoggerCtx::get_logger(), spdlog::level::debug, __VA_ARGS__)
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define INFO(...) \
  SPDLOG_LOGGER_CALL(LoggerCtx::get_logger(), spdlog::level::info, __VA_ARGS__)
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define WARN(...) \
  SPDLOG_LOGGER_CALL(LoggerCtx::get_logger(), spdlog::level::warn, __VA_ARGS__)
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define ERROR(...) \
  SPDLOG_LOGGER_CALL(LoggerCtx::get_logger(), spdlog::level::err, __VA_ARGS__)

class LoggerCtx {
 public:
  static auto get_logger() -> std::shared_ptr<spdlog::logger>;
};

#else

#define TRACE(...) (void)0
#define DEBUG(...) (void)0
#define INFO(...) (void)0
#define WARN(...) (void)0
#define ERROR(...) (void)0

#endif

#endif  // XYCO_UTILS_LOGGER_H_