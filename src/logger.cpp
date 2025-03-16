#include "logger.h"
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/basic_file_sink.h>
std::shared_ptr<spdlog::logger> g_logger;
void init_logger() {
    auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
    auto file_sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>("TurboTalkText.log", true);
    g_logger = std::make_shared<spdlog::logger>("app", spdlog::sinks_init_list{console_sink, file_sink});
    g_logger->set_level(spdlog::level::debug);
}
