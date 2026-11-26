#pragma once

#include <spdlog/logger.h>

#include <filesystem>
#include <memory>

namespace thj::core {

class Logger final {
  public:
    Logger();
    ~Logger() noexcept;

    Logger(const Logger &) = delete;
    Logger &operator=(const Logger &) = delete;

    Logger(Logger &&) = delete;
    Logger &operator=(Logger &&) = delete;

    void enableFileLogging(const std::filesystem::path &directory);

    spdlog::logger &get() noexcept;

  private:
    std::shared_ptr<spdlog::logger> logger_;
};

} // namespace thj::core
