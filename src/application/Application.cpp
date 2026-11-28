#include "application/Application.hpp"

#include "simulation/Components.hpp"

#include <chrono>
#include <cmath>
#include <stdexcept>
#include <string>

namespace thj::application {

namespace {

constexpr int kWindowWidth = 960;
constexpr int kWindowHeight = 540;

constexpr float kDemoEntitySize = 16.0F;

constexpr const char *kWindowTitle = "The Hollow Japan";

constexpr auto kStatsInterval = std::chrono::seconds{1};

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
    logger_.get().info("Simulation frequency: {} Hz", core::kSimulationHz);

    initializeSimulationDemo();
}

void Application::initializeSimulationDemo() {
    const core::Position2 initialPosition{core::FixedPoint::fromPixels(32),
                                          core::FixedPoint::fromPixels(64)};

    const auto entity = world_.queueCreate();

    world_.queueEmplace<simulation::TransformComponent>(entity, initialPosition, initialPosition);

    world_.queueEmplace<simulation::VelocityComponent>(entity, core::FixedPoint::fromPixels(40),
                                                       core::FixedPoint::fromPixels(0));

    world_.queueEmplace<simulation::LifetimeComponent>(entity, core::u64{600});

    logger_.get().info("Simulation demo queued: "
                       "speed=40 px/s, lifetime=600 ticks");
}

int Application::run() {
    auto previousTime = core::MonotonicClock::now();

    while (running_) {
        const auto currentTime = core::MonotonicClock::now();

        const auto elapsed =
            std::chrono::duration_cast<core::Nanoseconds>(currentTime - previousTime);

        previousTime = currentTime;

        pauseChanged_ = false;

        processEvents();

        if (!running_) {
            break;
        }

        const auto simulationDelta = pauseChanged_ ? core::Nanoseconds::zero() : elapsed;

        timestep_.beginFrame(simulationDelta);

        while (timestep_.hasPendingTick()) {
            const auto tick = timestep_.nextTick();

            fixedUpdate(tick);

            timestep_.consumeTick();
        }

        const auto timing = timestep_.endFrame();

        render(timing.interpolationAlpha);

        logFrameTiming(timing, elapsed);

        if (!vsyncEnabled_) {
            SDL_Delay(16);
        }
    }

    logger_.get().info("Application shutting down at simulation tick {}",
                       world_.currentTick().value);

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

            if (event.key.key == SDLK_Q && !event.key.repeat) {
                togglePause();
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

void Application::fixedUpdate(core::SimulationTick tick) {
    const auto report = world_.step(tick);

    snapRender_ = false;

    if (report.beginBarrier.total() > 0) {
        logger_.get().debug("Begin barrier | Tick: {} | "
                            "Applied: {} | Skipped: {}",
                            tick.value, report.beginBarrier.applied, report.beginBarrier.skipped);
    }

    if (report.endBarrier.total() > 0) {
        logger_.get().debug("End barrier | Tick: {} | "
                            "Applied: {} | Skipped: {}",
                            tick.value, report.endBarrier.applied, report.endBarrier.skipped);
    }
}

void Application::render(double interpolationAlpha) {
    const bool paused = timestep_.isPaused();

    const Uint8 red = paused ? 42 : 22;
    const Uint8 green = paused ? 30 : 24;
    const Uint8 blue = paused ? 44 : 32;

    if (!SDL_SetRenderDrawColor(renderer_.get(), red, green, blue, 255) ||
        !SDL_RenderClear(renderer_.get())) {
        throw std::runtime_error(std::string("Render clear failed: ") + SDL_GetError());
    }

    if (!SDL_SetRenderDrawColor(renderer_.get(), 120, 200, 151, 255)) {
        throw std::runtime_error(std::string("Render color failed: ") + SDL_GetError());
    }

    const double alpha = paused || snapRender_ ? 1.0 : interpolationAlpha;

    const auto &registry = world_.registry();

    registry.each<simulation::TransformComponent>(
        [&](core::EntityHandle, const simulation::TransformComponent &transform) {
            const double x = std::lerp(transform.previousPosition.x.toDouble(),
                                       transform.position.x.toDouble(), alpha);

            const double y = std::lerp(transform.previousPosition.y.toDouble(),
                                       transform.position.y.toDouble(), alpha);

            const SDL_FRect rectangle{static_cast<float>(x), static_cast<float>(y), kDemoEntitySize,
                                      kDemoEntitySize};

            if (!SDL_RenderFillRect(renderer_.get(), &rectangle)) {
                throw std::runtime_error(std::string("Entity rendering failed: ") + SDL_GetError());
            }
        });

    if (!SDL_RenderPresent(renderer_.get())) {
        throw std::runtime_error(std::string("Render present failed: ") + SDL_GetError());
    }
}

void Application::logFrameTiming(const core::FrameTiming &timing, core::Nanoseconds elapsed) {
    statsElapsed_ += elapsed;

    ++renderedFrames_;

    simulatedTicks_ += timing.executedTicks;

    droppedTicks_ += timing.droppedTicks;

    if (timing.deltaClamped && !timing.paused) {
        ++clampedFrames_;
    }

    if (statsElapsed_ < kStatsInterval) {
        return;
    }

    const double seconds = std::chrono::duration<double>(statsElapsed_).count();

    const double fps = static_cast<double>(renderedFrames_) / seconds;

    const double simulationRate = static_cast<double>(simulatedTicks_) / seconds;

    logger_.get().debug("FPS: {:.1f} | Sim: {:.1f} Hz | "
                        "Tick: {} | Alpha: {:.3f} | "
                        "Entities: {} | Pools: {} | "
                        "Dropped debt: {} | Clamped frames: {} | "
                        "Paused: {}",
                        fps, simulationRate, world_.currentTick().value, timing.interpolationAlpha,
                        world_.registry().aliveCount(), world_.registry().poolCount(),
                        droppedTicks_, clampedFrames_, timestep_.isPaused());

    if (droppedTicks_ > 0 || clampedFrames_ > 0) {
        logger_.get().warn("Simulation overload: {} dropped ticks "
                           "of debt, {} clamped frames",
                           droppedTicks_, clampedFrames_);
    }

    statsElapsed_ = core::Nanoseconds::zero();

    renderedFrames_ = 0;
    simulatedTicks_ = 0;
    droppedTicks_ = 0;
    clampedFrames_ = 0;
}

void Application::togglePause() {
    timestep_.setPaused(!timestep_.isPaused());

    pauseChanged_ = true;

    snapRender_ = true;

    logger_.get().info("Simulation {} at tick {}", timestep_.isPaused() ? "paused" : "resumed",
                       world_.currentTick().value);
}

} // namespace thj::application