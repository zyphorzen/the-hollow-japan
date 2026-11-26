find_package(Catch2 3 CONFIG REQUIRED)

add_executable(thj_tests
    unit/core/logging/LoggerTests.cpp
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
