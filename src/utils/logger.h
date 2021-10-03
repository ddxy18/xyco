#ifndef XYWEBSERVER_UTILS_LOGGER_H_
#define XYWEBSERVER_UTILS_LOGGER_H_

#include "spdlog/sinks/daily_file_sink.h"
#include "spdlog/sinks/stdout_color_sinks.h"
#include "spdlog/spdlog.h"

#undef SPDLOG_ACTIVE_LEVEL
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define SPDLOG_ACTIVE_LEVEL SPDLOG_LEVEL_TRACE

#undef TRACE
#undef DEBUG
#undef INFO
#undef WARN
#undef ERROR

#ifndef XYWEBSERVER_TEST
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
#else
#define TRACE(...) (void)0
#define DEBUG(...) (void)0
#define INFO(...) (void)0
#define WARN(...) (void)0
#define ERROR(...) (void)0
#endif

class LoggerCtx {
 public:
  static auto get_logger() -> std::shared_ptr<spdlog::logger> {
    auto f = []() {
      auto console_sink =
          std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
      console_sink->set_level(spdlog::level::level_enum::debug);
      console_sink->set_pattern("[%C-%m-%d %T.%e] [%l] [%t] [%@] %v");
      auto file_sink = std::make_shared<spdlog::sinks::daily_file_sink_mt>(
          "/var/log/xyco/runtime.txt", 0, 0, true);
      file_sink->set_level(spdlog::level::level_enum::trace);

      auto logger = std::make_shared<spdlog::logger>(
          spdlog::logger("xyco", {console_sink, file_sink}));
      logger->set_level(spdlog::level::level_enum::trace);

      return logger;
    };

    static auto console = f();

    return console;
  }
};

#endif  // XYWEBSERVER_UTILS_LOGGER_H_