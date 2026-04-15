# Order Book Engine

A high-performance limit order book engine implemented in C++20, featuring Market By Price (MBP) order matching with FIFO priority at each price level.

## Features

- **MBP Matching** — orders matched across all price levels, not just best bid/offer
- **FIFO Priority** — orders at the same price level are filled in arrival order
- **O(1) Cancel** — orders cancelled in constant time via ID lookup
- **Partial Fills** — incoming orders consume resting liquidity and rest any unfilled remainder
- **Self-Trade Prevention (STP)** — incoming orders that would match against the same user's resting orders are cancelled

## Usage

```cpp
OrderBook book;

book.addOrder(Order{1, Side::SELL, 100, 10, /*userId=*/42}); // rests at 100
book.addOrder(Order{2, Side::SELL, 101, 5,  /*userId=*/42}); // rests at 101
AddResult r = book.addOrder(Order{3, Side::BUY, 100, 10, /*userId=*/7}); // fills against order 1
book.cancelOrder(2); // removes order 2 from the book
```

`addOrder` returns `ADDED`, `FILLED`, or `STP_CANCELLED`.

## Design Decisions

### `PriceLevel` — `std::list` + `std::unordered_map`
Each price level keeps orders in a `std::list` for stable FIFO ordering. An `unordered_map<OrderId, iterator>` maps each order ID to its position in the list, so cancellation is O(1) without shifting the queue.

### `OrderBook` — `std::map<Price, PriceLevel>`
Both the bid and ask sides use a sorted `std::map`. This makes finding the best bid (highest) and best ask (lowest) a single iterator dereference, and sweep-matching just walks the map in order without any extra sorting.

### Cancel in O(1)
A top-level `unordered_map<OrderId, OrderLocation>` records which price and side each resting order lives at. A cancel looks up the location, jumps straight to the right `PriceLevel`, and removes the order — no scanning required.

### Matching
When an order arrives, `matchOrder` runs before the order is inserted:
- **Buy** — walks asks from lowest price upward while `ask.price <= buy.price`
- **Sell** — walks bids from highest price downward while `bid.price >= sell.price`

Within each level orders are consumed FIFO. Fully drained levels are erased from the map. Any unfilled remainder rests at the limit price.

### Self-Trade Prevention
If the incoming order would match against a resting order from the same user, matching halts immediately and `STP_CANCELLED` is returned. Fills that already occurred before hitting the self-order stand; the remainder is not rested.

## Build

Requires CMake 3.15+ and a C++20 compiler.

```bash
cmake -B build
cmake --build build
```

## Running Tests

```bash
ctest --test-dir build --output-on-failure
```

Run a specific tag:
```bash
./build/tests/Debug/tests.exe [matching]
./build/tests/Debug/tests.exe [cancel]
```

## Benchmarks

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release -DBUILD_BENCHMARKS=ON
cmake --build build --config Release
./build/benchmarks/Release/benchmarks.exe
```

### Results (Release, Windows, MSVC)

| Benchmark | Time | CPU |
|---|---|---|
| AddOrder (no match) | 421 ns | 389 ns |
| AddOrder (full match) | 448 ns | 273 ns |
| CancelOrder | 464 ns | 288 ns |
| SweepLevels/8 | 3.7 µs | 2.9 µs |
| SweepLevels/64 | 25 µs | 21 µs |
| SweepLevels/512 | 203 µs | 166 µs |
| SweepLevels/1024 | 430 µs | 310 µs |

Sweep scales linearly with levels consumed — expected O(n) behavior for MBP matching.
