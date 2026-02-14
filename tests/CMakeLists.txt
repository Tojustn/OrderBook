Include(FetchContent)

FetchContent_Declare(
  Catch2
  GIT_REPOSITORY https://github.com/catchorg/Catch2.git
  GIT_TAG        v3.8.1 # or a later release
)

FetchContent_MakeAvailable(Catch2)

add_executable(tests test.cpp)
target_link_libraries(tests PRIVATE orderbook Catch2::Catch2WithMain)
FetchContent_MakeAvailable(Catch2)


include(CTest)
include(Catch)
catch_discover_tests(order_book_tests)