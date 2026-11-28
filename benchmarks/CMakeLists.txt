add_executable(thj_ecs_benchmark
    EcsBenchmark.cpp
)

target_link_libraries(thj_ecs_benchmark PRIVATE
    thj_core
)

thj_enable_warnings(thj_ecs_benchmark)