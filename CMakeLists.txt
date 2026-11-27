cmake_minimum_required(VERSION 3.28)

project(
    TheHollowJapan
    VERSION 0.1.0
    DESCRIPTION "A Japanese fantasy open-world RPG"
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(THJ_BUILD_TESTS "Build automated tests" ON)

function(thj_enable_warnings target)
    if(MSVC)
        target_compile_options(${target} PRIVATE
            /W4
            /permissive-
        )
    else()
        target_compile_options(${target} PRIVATE
            -Wall
            -Wextra
            -Wpedantic
        )
    endif()
endfunction()

find_package(SDL3 CONFIG REQUIRED)
find_package(spdlog CONFIG REQUIRED)

add_library(thj_core STATIC
    src/core/logging/Logger.cpp
    src/core/time/FixedTimestep.cpp
    src/ecs/EntityManager.cpp
)

target_include_directories(thj_core PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/src"
)

target_compile_features(thj_core PUBLIC
    cxx_std_23
)

target_link_libraries(thj_core PUBLIC
    spdlog::spdlog
)

thj_enable_warnings(thj_core)

add_executable(thj_client
    src/client/main.cpp
    src/application/Application.cpp
    src/platform/SdlContext.cpp
)

target_link_libraries(thj_client PRIVATE
    thj_core
    SDL3::SDL3
)

thj_enable_warnings(thj_client)

if(THJ_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
