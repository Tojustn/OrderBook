add_executable(benchmarks order_book_bench.cpp)

target_include_directories(benchmarks
    PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)

target_link_libraries(benchmarks
    PRIVATE
    orderbook
)
