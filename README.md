# Order Book Engine

High-performance C++20 limit order book with Market By Price (MBP) matching, O(1) cancel, and nanosecond-resolution latency benchmarks.

## Features

- **MBP Matching** — orders matched across all price levels, not just best bid/offer
- **FIFO Priority** — orders at the same price level filled in arrival order
- **O(1) Cancel** — constant-time cancellation via ID lookup
- **Partial Fills** — incoming orders consume resting liquidity and rest any unfilled remainder
- **Self-Trade Prevention** — orders matching against the same user's resting orders are cancelled immediately

## Usage

```cpp
OrderBook book;

// Rest orders
book.addOrder(Order{1, Side::SELL, 100, 10, /*userId=*/42});
book.addOrder(Order{2, Side::SELL, 101, 5,  /*userId=*/42});

// Match — fills against order 1 at price 100
AddResult r = book.addOrder(Order{3, Side::BUY, 100, 10, /*userId=*/7});
// r == AddResult::FILLED

// Cancel
book.cancelOrder(2);
```

## Design

### Price levels — intrusive linked list
Each price level maintains orders in an intrusive doubly-linked list — `next_`/`prev_` pointers live directly on `Order`, eliminating the allocator overhead of `std::list` nodes. An `unordered_map<OrderId, Order*>` maps each ID to its node for O(1) removal without traversal.

### Order book sides — `std::map<Price, PriceLevel>`
Both bid and ask sides use a sorted `std::map`. Best bid (highest) and best ask (lowest) are a single iterator dereference. Sweep-matching walks the map in order with no extra sorting.

### Memory pool — `OrderPool`
`OrderPool` maintains a free list of recycled `Order` slots chained through their intrinsic `next_` pointer. Filled and cancelled orders are returned to the pool rather than deallocated; new orders are placement-newed into a recycled slot. This eliminates per-order heap allocation in steady state.

### O(1) cancel
A top-level `unordered_map<OrderId, Order*>` maps each live order ID directly to its `Order*`. Cancel dereferences the pointer to get price and side, jumps to the right `PriceLevel`, unlinks the node, and returns the slot to the pool — no scanning, no extra metadata struct.

### Matching
When an order arrives, `matchOrder` runs before insertion:
- **Buy** — walks asks from lowest price upward while `ask.price <= buy.price`
- **Sell** — walks bids from highest price downward while `bid.price >= sell.price`

Orders within each level are consumed FIFO. Fully drained levels are erased from the map. Any unfilled remainder rests at the limit price.

### Self-trade prevention
If an incoming order would match against a resting order from the same user, matching halts and `STP_CANCELLED` is returned. Prior fills stand; the remainder is not rested.

## Build

Requires CMake 3.15+ and a C++20 compiler.

```bash
cmake -B build
cmake --build build
```

## Tests

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

Benchmarks use a hand-rolled `rdtsc` harness — 100,000 per-operation samples, converted to nanoseconds via a 100ms wall-clock calibration window.

**Warm** benchmarks construct one book outside the timed loop with resting orders pre-loaded, reflecting steady-state conditions. **Cold** benchmarks reconstruct per iteration — unavoidable where the book empties on each operation (full match, sweep).

### Results (Release, Linux/WSL2, GCC) on 4/17

| Benchmark | Cache | p50 | p99 | p99.9 |
|---|---|---|---|---|
| addOrder (no match) | warm | 60 ns | 2,545 ns | 6,061 ns |
| addOrder (full match) | cold | 50 ns | 261 ns | 631 ns |
| cancelOrder | warm | 80 ns | 581 ns | 1,433 ns |
| sweep 8 levels | cold | 471 ns | 952 ns | 1,723 ns |
| sweep 64 levels | cold | 3,497 ns | 7,835 ns | 42,392 ns |
| sweep 256 levels | cold | 14,107 ns | 36,250 ns | 94,603 ns |
| sweep 1024 levels | cold | 55,828 ns | 143,968 ns | 304,758 ns |

p99 spike on `addOrder (no match)` (60ns → 2,545ns) reflects WSL2 scheduler jitter, not book logic

Sweep latency scales linearly with levels consumed — expected O(n) behaviour for MBP matching across `std::map` price levels.