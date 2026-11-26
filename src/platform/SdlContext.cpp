#include "platform/SdlContext.hpp"

#include <SDL3/SDL.h>

#include <stdexcept>
#include <string>

namespace thj::platform {

SdlContext::SdlContext() {
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        const std::string error = SDL_GetError();

        SDL_Quit();

        throw std::runtime_error("Failed to initialize SDL: " + error);
    }
}

SdlContext::~SdlContext() noexcept { SDL_Quit(); }

} // namespace thj::platform
