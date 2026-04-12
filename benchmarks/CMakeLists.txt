# Fetch Google Benchmark from GitHub
include(FetchContent)

FetchContent_Declare(
    benchmark
    GIT_REPOSITORY https://github.com/google/benchmark.git
    GIT_TAG v1.8.3
    GIT_SHALLOW TRUE
)

set(BENCHMARK_ENABLE_TESTING OFF CACHE BOOL "" FORCE)

FetchContent_MakeAvailable(benchmark)

add_executable(benchmarks
    order_book_bench.cpp
)

target_include_directories(benchmarks
    PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)

target_link_libraries(benchmarks
    PRIVATE
    orderbook
    benchmark::benchmark
)