
#include "ecs/CommandBuffer.hpp"
#include "ecs/Registry.hpp"

#include <algorithm>
#include <array>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <functional>
#include <iomanip>
#include <iostream>
#include <stdexcept>
#include <utility>

namespace {

using thj::core::EntityHandle;

using thj::ecs::CommandBuffer;
using thj::ecs::Registry;

using Clock = std::chrono::steady_clock;

constexpr std::size_t kSamples = 5;
constexpr std::size_t kQueryIterations = 60;

struct Position {
    std::int64_t x{};
    std::int64_t y{};
};

struct Velocity {
    std::int64_t x{};
    std::int64_t y{};
};

struct BenchmarkResult {
    double creationMs{};
    double denseQueryMs{};
    double orderedQueryMs{};
    double recordingMs{};
    double playbackMs{};

    std::int64_t checksum{};
};

template <typename Fn> double measureMilliseconds(Fn &&callback) {
    const auto start = Clock::now();

    std::invoke(callback);

    const auto finish = Clock::now();

    return std::chrono::duration<double, std::milli>(finish - start).count();
}

void requireCondition(bool condition, const char *message) {
    if (!condition) {
        throw std::runtime_error(message);
    }
}

BenchmarkResult runWorkload(std::size_t entityCount) {
    Registry registry;
    CommandBuffer commands{registry};

    BenchmarkResult result;

    registry.reserveEntities(entityCount);

    registry.reserveComponents<Position>(entityCount);

    registry.reserveComponents<Velocity>(entityCount);

    commands.reserve(entityCount / 2);

    result.creationMs = measureMilliseconds([&] {
        for (std::size_t i = 0; i < entityCount; ++i) {
            const auto entity = registry.create();

            registry.emplace<Position>(entity, static_cast<std::int64_t>(i), std::int64_t{0});

            registry.emplace<Velocity>(entity, std::int64_t{2}, std::int64_t{3});
        }
    });

    requireCondition(registry.aliveCount() == entityCount,
                     "Unexpected entity count after creation");

    std::size_t denseVisits = 0;

    result.denseQueryMs = measureMilliseconds([&] {
        for (std::size_t iteration = 0; iteration < kQueryIterations; ++iteration) {
            denseVisits += registry.each<Position, Velocity>(
                [](EntityHandle, Position &position, Velocity &velocity) {
                    position.x += velocity.x;
                    position.y += velocity.y;
                });
        }
    });

    result.denseQueryMs /= static_cast<double>(kQueryIterations);

    requireCondition(denseVisits == entityCount * kQueryIterations,
                     "Dense query visit count mismatch");

    std::size_t orderedVisits = 0;

    result.orderedQueryMs = measureMilliseconds([&] {
        for (std::size_t iteration = 0; iteration < kQueryIterations; ++iteration) {
            orderedVisits += registry.eachOrdered<Position, Velocity>(
                [](EntityHandle, Position &position, Velocity &velocity) {
                    position.x += velocity.x;
                    position.y += velocity.y;
                });
        }
    });

    result.orderedQueryMs /= static_cast<double>(kQueryIterations);

    requireCondition(orderedVisits == entityCount * kQueryIterations,
                     "Ordered query visit count mismatch");

    result.recordingMs = measureMilliseconds([&] {
        registry.each<Position>([&](EntityHandle entity, const Position &) {
            if (entity.index % 2 == 0) {
                commands.destroy(entity);
            }
        });
    });

    requireCondition(commands.size() == entityCount / 2, "Incorrect number of queued commands");

    std::size_t applied = 0;

    result.playbackMs = measureMilliseconds([&] {
        const auto playback = commands.playback();

        playback.rethrowIfFailed();

        applied = playback.applied;
    });

    requireCondition(applied == entityCount / 2, "Unexpected number of applied commands");

    requireCondition(registry.aliveCount() == entityCount / 2, "Incorrect surviving entity count");

    requireCondition(registry.componentCount<Position>() == entityCount / 2 &&
                         registry.componentCount<Velocity>() == entityCount / 2,
                     "Incorrect surviving component count");

    const auto &readOnly = std::as_const(registry);

    const auto survivors = readOnly.each<Position, Velocity>(
        [&](EntityHandle entity, const Position &position, const Velocity &) {
            requireCondition(entity.index % 2 != 0, "An entity scheduled for destruction survived");

            const auto expectedX = static_cast<std::int64_t>(entity.index) +
                                   static_cast<std::int64_t>(kQueryIterations * 4);

            const auto expectedY = static_cast<std::int64_t>(kQueryIterations * 6);

            requireCondition(position.x == expectedX && position.y == expectedY,
                             "Benchmark component state mismatch");

            result.checksum += position.x + position.y;
        });

    requireCondition(survivors == entityCount / 2, "Survivor query count mismatch");

    return result;
}

template <typename Projection>
double median(const std::array<BenchmarkResult, kSamples> &samples, Projection projection) {
    std::array<double, kSamples> values{};

    for (std::size_t i = 0; i < samples.size(); ++i) {
        values[i] = std::invoke(projection, samples[i]);
    }

    std::sort(values.begin(), values.end());

    return values[values.size() / 2];
}

void printWorkload(std::size_t entityCount) {
    (void)runWorkload(entityCount);

    std::array<BenchmarkResult, kSamples> samples{};

    for (auto &sample : samples) {
        sample = runWorkload(entityCount);
    }

    for (const auto &sample : samples) {
        requireCondition(sample.checksum == samples.front().checksum,
                         "Benchmark checksum is not reproducible");
    }

    std::cout << entityCount << ',' << median(samples, &BenchmarkResult::creationMs) << ','
              << median(samples, &BenchmarkResult::denseQueryMs) << ','
              << median(samples, &BenchmarkResult::orderedQueryMs) << ','
              << median(samples, &BenchmarkResult::recordingMs) << ','
              << median(samples, &BenchmarkResult::playbackMs) << ',' << samples.front().checksum
              << '\n';
}

} // namespace

int main() {
    try {
#ifndef NDEBUG
        std::cerr << "Warning: benchmark is running "
                  << "without NDEBUG. Use a Release build.\n";
#endif

        constexpr std::array<std::size_t, 3> workloads{1'000, 10'000, 100'000};

        std::cout << std::fixed << std::setprecision(6);

        std::cout << "entities,"
                  << "creation_ms,"
                  << "dense_query_ms_per_pass,"
                  << "ordered_query_ms_per_pass,"
                  << "recording_ms,"
                  << "playback_ms,"
                  << "checksum\n";

        for (const auto count : workloads) {
            printWorkload(count);
        }

        return 0;
    } catch (const std::exception &error) {
        std::cerr << "ECS benchmark failed: " << error.what() << '\n';

        return 1;
    }
}
