#include "logger.h"
#include <spdlog/sinks/stdout_color_sinks.h>

void Logger::init() {
    auto console = spdlog::stdout_color_mt("console");
    spdlog::set_default_logger(console);
    spdlog::set_level(spdlog::level::info);
}

void Logger::info(const std::string& message) {
    spdlog::info(message);
}

void Logger::error(const std::string& message) {
    spdlog::error(message);
}
