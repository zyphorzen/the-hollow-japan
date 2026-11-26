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

add_executable(thj_client
    src/client/main.cpp
)

target_compile_features(thj_client PRIVATE cxx_std_23)

if(MSVC)
    target_compile_options(thj_client PRIVATE
        /W4
        /permissive-
    )
else()
    target_compile_options(thj_client PRIVATE
        -Wall
        -Wextra
        -Wpedantic
    )
endif()
