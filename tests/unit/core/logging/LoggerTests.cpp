#include "core/logging/Logger.hpp"

#include <catch2/catch_test_macros.hpp>

#include <filesystem>
#include <fstream>
#include <iterator>
#include <random>
#include <stdexcept>
#include <string>
#include <system_error>
#include <utility>

namespace {

class TempDirectory final {
  public:
    TempDirectory() {
        std::random_device random;

        for (int attempt = 0; attempt < 16; ++attempt) {
            auto candidate = std::filesystem::temp_directory_path() /
                             ("thj-logger-test-" + std::to_string(random()));

            if (std::filesystem::create_directory(candidate)) {
                path_ = std::move(candidate);
                return;
            }
        }

        throw std::runtime_error("Cannot create test directory");
    }

    ~TempDirectory() noexcept {
        std::error_code error;
        std::filesystem::remove_all(path_, error);
    }

    TempDirectory(const TempDirectory &) = delete;
    TempDirectory &operator=(const TempDirectory &) = delete;

    const std::filesystem::path &path() const noexcept { return path_; }

  private:
    std::filesystem::path path_;
};

} // namespace

TEST_CASE("Logger starts with one console sink", "[unit][logging]") {
    thj::core::Logger logger;

    REQUIRE(logger.get().name() == "THJ");
    REQUIRE(logger.get().sinks().size() == 1);
}

TEST_CASE("Logger uses the configured build log level", "[unit][logging]") {
    thj::core::Logger logger;

#ifdef NDEBUG
    CHECK_FALSE(logger.get().should_log(spdlog::level::debug));
#else
    CHECK(logger.get().should_log(spdlog::level::debug));
#endif

    CHECK(logger.get().should_log(spdlog::level::info));

    CHECK(logger.get().should_log(spdlog::level::critical));

    CHECK_FALSE(logger.get().should_log(spdlog::level::trace));
}

TEST_CASE("Logger persists messages to a file", "[integration][logging]") {
    TempDirectory directory;

    const auto logFile = directory.path() / "thj.log";

    {
        thj::core::Logger logger;

        logger.enableFileLogging(directory.path());

        REQUIRE(logger.get().sinks().size() == 2);

        logger.get().info("THJ_TEST_FILE_MESSAGE");
    }

    std::ifstream input(logFile);

    REQUIRE(input.is_open());

    const std::string content((std::istreambuf_iterator<char>(input)),
                              std::istreambuf_iterator<char>());

    CHECK(content.find("THJ_TEST_FILE_MESSAGE") != std::string::npos);
}
