#include "application/Application.hpp"

#include <SDL3/SDL_main.h>

#include <exception>
#include <iostream>

int main(int argc, char *argv[]) {
    (void)argc;
    (void)argv;

    try {
        thj::application::Application application;

        return application.run();
    } catch (const std::exception &error) {
        std::cerr << "[FATAL] " << error.what() << '\n';

        return 1;
    }
}
