#include "core/logging/Logger.hpp"

#include <spdlog/sinks/rotating_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>

namespace thj::core {

namespace {

constexpr std::size_t kMaxFileSize = 5 * 1024 * 1024;
constexpr std::size_t kMaxBackups = 3;
constexpr const char *kPattern = "[%Y-%m-%d %H:%M:%S.%e] [%^%l%$] [%n] %v";

} // namespace

Logger::Logger() {
    auto console = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();

    logger_ = std::make_shared<spdlog::logger>("THJ", console);

    logger_->set_pattern(kPattern);

#ifdef NDEBUG
    logger_->set_level(spdlog::level::info);
#else
    logger_->set_level(spdlog::level::debug);
#endif

    logger_->flush_on(spdlog::level::warn);
}

Logger::~Logger() noexcept {
    try {
        logger_->flush();
    } catch (...) {
    }
}

void Logger::enableFileLogging(const std::filesystem::path &directory) {
    std::filesystem::create_directories(directory);

    auto file = std::make_shared<spdlog::sinks::rotating_file_sink_mt>(
        (directory / "thj.log").string(), kMaxFileSize, kMaxBackups);

    file->set_pattern(kPattern);

    logger_->sinks().push_back(file);
}

spdlog::logger &Logger::get() noexcept { return *logger_; }

} // namespace thj::core
