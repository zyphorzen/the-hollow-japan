
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>

#include <iostream>

int main(int argc, char* argv[]) {
    (void)argc;
    (void)argv;

    if (!SDL_Init(SDL_INIT_VIDEO)) {
        std::cerr << "SDL initialization failed: "
                  << SDL_GetError() << '\n';

        SDL_Quit();
        return 1;
    }

    const int version = SDL_GetVersion();
    const char* driver = SDL_GetCurrentVideoDriver();

    std::cout << "The Hollow Japan\n"
              << "Beyond the living world, something waits.\n"
              << "SDL version: "
              << SDL_VERSIONNUM_MAJOR(version) << '.'
              << SDL_VERSIONNUM_MINOR(version) << '.'
              << SDL_VERSIONNUM_MICRO(version) << '\n'
              << "Video driver: "
              << (driver ? driver : "unknown") << '\n'
              << "SDL3 smoke test passed\n";

    SDL_Quit();

    return 0;
}
