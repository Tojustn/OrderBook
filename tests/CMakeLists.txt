  include(FetchContent)

  FetchContent_Declare(
    Catch2
    GIT_REPOSITORY https://github.com/catchorg/Catch2.git
    GIT_TAG        v3.8.1
  )

  FetchContent_MakeAvailable(Catch2)

  add_executable(tests price_level_test.cpp order_book_test.cpp)
  target_link_libraries(tests PRIVATE orderbook Catch2::Catch2WithMain)

  include(CTest)
  include(Catch)
  catch_discover_tests(tests)