find_package(Catch2 3 CONFIG REQUIRED)

add_executable(thj_tests
    unit/core/logging/LoggerTests.cpp

    unit/core/TypesTests.cpp

    unit/core/math/FixedPointTests.cpp

    unit/core/world/CoordinatesTests.cpp

    unit/core/time/SimulationTimeTests.cpp
    unit/core/time/FixedTimestepTests.cpp

    unit/ecs/EntityManagerTests.cpp
    unit/ecs/SparseSetTests.cpp
    unit/ecs/ComponentStorageTests.cpp
    unit/ecs/RegistryTests.cpp
    unit/ecs/QueryTests.cpp
    unit/ecs/CommandBufferTests.cpp

    unit/simulation/MovementSystemTests.cpp
    unit/simulation/LifetimeSystemTests.cpp
    unit/simulation/SimulationWorldTests.cpp

    unit/ecs/EntityGenerationTests.cpp
    unit/ecs/EcsInvariantTests.cpp
    unit/simulation/DeterministicReplayTests.cpp
)

target_include_directories(thj_tests PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(thj_tests PRIVATE
    thj_core
    Catch2::Catch2WithMain
)

thj_enable_warnings(thj_tests)

include(Catch)

catch_discover_tests(
    thj_tests
    DISCOVERY_MODE PRE_TEST
)