#pragma once

#include "core/logging/Logger.hpp"
#include "platform/SdlContext.hpp"
#include <SDL3/SDL.h>
#include <memory>

namespace thj::application {

class Application final {
  public:
    explicit Application(core::Logger &logger);
    ~Application() = default;

    Application(const Application &) = delete;
    Application &operator=(const Application &) = delete;

    Application(Application &&) = delete;
    Application &operator=(Application &&) = delete;

    int run();

  private:
    struct WindowDeleter {
        void operator()(SDL_Window *window) const noexcept;
    };

    struct RendererDeleter {
        void operator()(SDL_Renderer *renderer) const noexcept;
    };

    using WindowPtr = std::unique_ptr<SDL_Window, WindowDeleter>;

    using RendererPtr = std::unique_ptr<SDL_Renderer, RendererDeleter>;

    void processEvents();
    void render();

    core::Logger &logger_;

    platform::SdlContext sdlContext_;
    WindowPtr window_;
    RendererPtr renderer_;

    bool running_{true};
    bool vsyncEnabled_{false};
};

} // namespace thj::application
