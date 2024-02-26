module;

#include "spdlog/cfg/env.h"
#include "spdlog/sinks/daily_file_sink.h"
#include "spdlog/sinks/stdout_color_sinks.h"
#include "spdlog/spdlog.h"

module xyco.logging;

auto xyco::logging::LoggerCtx::get_logger() -> std::shared_ptr<spdlog::logger> {
  auto configure = []() {
    spdlog::cfg::load_env_levels();

    auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
    console_sink->set_pattern("[%C-%m-%d %T.%e] [%l] [%t] [%@] %v");
    auto file_sink = std::make_shared<spdlog::sinks::daily_file_sink_mt>(
        "runtime.txt", 0, 0, true);

    auto logger = std::make_shared<spdlog::logger>(
        spdlog::logger("xyco", {console_sink, file_sink}));
    logger->set_level(spdlog::get_level());
    logger->set_pattern("[%C-%m-%d %T.%e] [%l] [%t] [%@] %v");

    return logger;
  };

  static auto console = configure();

  return console;
}
