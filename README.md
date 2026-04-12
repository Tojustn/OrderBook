# Order Book Engine

A high-performance limit order book engine implemented in C++20, featuring Market By Price (MBP) order matching with FIFO priority at each price level.

## Features

- **MBP Matching** — orders matched across all price levels, not just best bid/offer
- **FIFO Priority** — orders at the same price level are filled in arrival order
- **O(1) Cancel** — orders cancelled in constant time via ID lookup
- **Partial Fills** — incoming orders consume resting liquidity and rest any unfilled remainder

## Architecture

### `Order`
Immutable value type holding order id, side, price, and quantity.

### `PriceLevel`
Manages all orders resting at a single price. Uses a `std::list` for FIFO ordering and an `std::unordered_map<OrderId, iterator>` for O(1) cancellation without disturbing queue position.

### `OrderBook`
Top-level book with separate bid and ask sides. Uses `std::map<Price, PriceLevel>` for both sides — ordered iteration is free, giving cheapest ask and highest bid in O(log n).

An `unordered_map<OrderId, OrderLocation>` tracks which price level each resting order lives at, enabling O(1) cancel lookups across the book.

### Matching
When an order arrives, `matchOrder` is called before insertion:
- **Buy** — iterates asks from lowest price upward, matching while `ask.price <= buy.price`
- **Sell** — iterates bids from highest price downward, matching while `bid.price >= sell.price`

Within each level, orders are consumed FIFO. Fully drained levels are removed from the map. Any unfilled remainder rests in the book at the limit price.

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
