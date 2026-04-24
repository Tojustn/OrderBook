add_executable(replay replay_bench.cpp)

target_include_directories(replay
    PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)

target_link_libraries(replay
    PRIVATE
    orderbook
)
