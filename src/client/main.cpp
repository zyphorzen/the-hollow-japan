#include "application/Application.hpp"
#include "core/logging/Logger.hpp"

#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>

#include <exception>
#include <filesystem>
#include <iostream>
#include <memory>
#include <stdexcept>

int main(int argc, char *argv[]) {
    (void)argc;
    (void)argv;

    try {
        thj::core::Logger logger;

        try {
            std::unique_ptr<char, decltype(&SDL_free)> path(
                SDL_GetPrefPath("THJ", "TheHollowJapan"), &SDL_free);

            if (!path) {
                throw std::runtime_error(SDL_GetError());
            }

            const auto directory = std::filesystem::path(path.get()) / "logs";

            logger.enableFileLogging(directory);

            logger.get().info("Log directory: {}", directory.string());
        } catch (const std::exception &error) {
            logger.get().warn("File logging unavailable: {}", error.what());
        }

        try {
            logger.get().debug("Debug logging enabled");

            thj::application::Application application(logger);

            return application.run();
        } catch (const std::exception &error) {
            logger.get().critical("Fatal application error: {}", error.what());

            return 1;
        }
    } catch (const std::exception &error) {
        std::cerr << "[FATAL] Logging/bootstrap failure: " << error.what() << '\n';

        return 1;
    }
}
