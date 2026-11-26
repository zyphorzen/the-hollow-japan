#include "application/Application.hpp"

#include <stdexcept>
#include <string>

namespace thj::application {

namespace {

constexpr int kWindowWidth = 960;
constexpr int kWindowHeight = 540;

constexpr const char *kWindowTitle = "The Hollow Japan";

} // namespace

void Application::WindowDeleter::operator()(SDL_Window *window) const noexcept {
    SDL_DestroyWindow(window);
}

void Application::RendererDeleter::operator()(SDL_Renderer *renderer) const noexcept {
    SDL_DestroyRenderer(renderer);
}

Application::Application(core::Logger &logger) : logger_(logger) {
    window_.reset(SDL_CreateWindow(kWindowTitle, kWindowWidth, kWindowHeight,
                                   SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIGH_PIXEL_DENSITY));

    if (!window_) {
        throw std::runtime_error(std::string("Failed to create window: ") + SDL_GetError());
    }

    renderer_.reset(SDL_CreateRenderer(window_.get(), nullptr));

    if (!renderer_) {
        throw std::runtime_error(std::string("Failed to create renderer: ") + SDL_GetError());
    }

    vsyncEnabled_ = SDL_SetRenderVSync(renderer_.get(), 1);

    if (!vsyncEnabled_) {
        logger_.get().warn("VSync unavailable: {}", SDL_GetError());
    }

    const char *videoDriver = SDL_GetCurrentVideoDriver();
    const char *rendererName = SDL_GetRendererName(renderer_.get());

    logger_.get().info("The Hollow Japan initialized");
    logger_.get().info("Video driver: {}", videoDriver ? videoDriver : "unknown");
    logger_.get().info("Renderer: {}", rendererName ? rendererName : "unknown");
    logger_.get().info("VSync: {}", vsyncEnabled_ ? "enabled" : "disabled");
}

int Application::run() {
    while (running_) {
        processEvents();
        if (!running_) {
            break;
        }

        render();
        if (!vsyncEnabled_) {
            SDL_Delay(16);
        }
    }

    logger_.get().info("Application shutting down");

    return 0;
}

void Application::processEvents() {
    SDL_Event event{};

    while (SDL_PollEvent(&event)) {
        switch (event.type) {
        case SDL_EVENT_QUIT:
        case SDL_EVENT_WINDOW_CLOSE_REQUESTED:
            running_ = false;
            break;

        case SDL_EVENT_KEY_DOWN:
            if (event.key.key == SDLK_ESCAPE) {
                running_ = false;
            }
            break;

        case SDL_EVENT_WINDOW_RESIZED:
            logger_.get().debug("Window resized: {}x{}", event.window.data1, event.window.data2);
            break;

        default:
            break;
        }
    }
}

void Application::render() {
    if (!SDL_SetRenderDrawColor(renderer_.get(), 22, 24, 32, 255) ||
        !SDL_RenderClear(renderer_.get()) || !SDL_RenderPresent(renderer_.get())) {
        throw std::runtime_error(std::string("Rendering failed: ") + SDL_GetError());
    }
}

} // namespace thj::application
