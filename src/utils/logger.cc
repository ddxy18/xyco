#ifdef XYCO_ENABLE_LOG

#include "logger.h"

#include "spdlog/sinks/daily_file_sink.h"
#include "spdlog/sinks/stdout_color_sinks.h"

auto LoggerCtx::get_logger() -> std::shared_ptr<spdlog::logger> {
  auto configure = []() {
    auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
    console_sink->set_level(spdlog::level::level_enum::debug);
    console_sink->set_pattern("[%C-%m-%d %T.%e] [%l] [%t] [%@] %v");
    auto file_sink = std::make_shared<spdlog::sinks::daily_file_sink_mt>(
        "/var/log/xyco/runtime.txt", 0, 0, true);

    auto logger = std::make_shared<spdlog::logger>(
        spdlog::logger("xyco", {console_sink, file_sink}));
    logger->set_level(spdlog::level::level_enum::trace);
    logger->set_pattern("[%C-%m-%d %T.%e] [%l] [%t] [%@] %v");

    return logger;
  };

  static auto console = configure();

  return console;
}

#endif