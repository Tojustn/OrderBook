#include <catch2/catch_test_macros.hpp>
#include <cstdint>
#include <spsc_queue.hpp>

TEST_CASE("SPSC_QUEUE.push -- Pushing is allowed on a non full queue") {
  SPSC_QUEUE<int64_t> queue{5};
  int64_t num{9};

  // Remove == because push(num) works and shows actual return
  REQUIRE(queue.push(num));
}

TEST_CASE("SPSC_QUEUE.push -- Pushing is not allowed on a full queue") {
  SPSC_QUEUE<int64_t> queue{5};
  int64_t nums[5]{1, 2, 3, 4, 5};
  for (int64_t num : nums) {
    REQUIRE(queue.push(num));
  }

  int64_t fail{1};
  REQUIRE_FALSE(queue.push(fail));
}
TEST_CASE("SPSC_QUEUE.pop -- popping is not allowed on an empty queue") {
  SPSC_QUEUE<int64_t> queue{5};
  int64_t out{};
  REQUIRE_FALSE(queue.pop(out));
}

TEST_CASE("SPSC_QUEUE.pop -- pops in FIFO order") {
  SPSC_QUEUE<int64_t> queue{5};
  for (int64_t num : {1, 2, 3, 4, 5})
    REQUIRE(queue.push(num));

  int64_t out{};
  for (int64_t expected : {1, 2, 3, 4, 5}) {
    REQUIRE(queue.pop(out));
    CHECK(out == expected);
  }
  REQUIRE_FALSE(queue.pop(out));
}

TEST_CASE("SPSC_QUEUE -- survives wrapping around the ring") {
  SPSC_QUEUE<int64_t> queue{5};
  int64_t out{};

  for (int64_t num : {1, 2, 3, 4, 5})
    REQUIRE(queue.push(num));
  for (int64_t expected : {1, 2, 3, 4, 5}) {
    REQUIRE(queue.pop(out));
    CHECK(out == expected);
  }

  for (int64_t num : {6, 7, 8, 9, 10})
    REQUIRE(queue.push(num));
  for (int64_t expected : {6, 7, 8, 9, 10}) {
    REQUIRE(queue.pop(out));
    CHECK(out == expected);
  }
}
