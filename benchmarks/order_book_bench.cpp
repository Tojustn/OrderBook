#include <benchmark/benchmark.h>
#include "order_book.hpp"
#include "order.hpp"
#include "types.hpp"

// Benchmark adding a buy order with no match
static void BM_AddOrderNoMatch(benchmark::State& state) {
    for (auto _ : state) {
        OrderBook book;
        book.addOrder(Order(1, Side::BUY, 100, 50, 1));
    }
}
BENCHMARK(BM_AddOrderNoMatch);

// Benchmark adding an order that fully matches
static void BM_AddOrderFullMatch(benchmark::State& state) {
    for (auto _ : state) {
        OrderBook book;
        book.addOrder(Order(1, Side::SELL, 100, 50, 1));
        book.addOrder(Order(2, Side::BUY, 100, 50, 2));
    }
}
BENCHMARK(BM_AddOrderFullMatch);

// Benchmark cancel order
static void BM_CancelOrder(benchmark::State& state) {
    for (auto _ : state) {
        OrderBook book;
        book.addOrder(Order(1, Side::BUY, 100, 50, 1));
        book.cancelOrder(1);
    }
}
BENCHMARK(BM_CancelOrder);

// Benchmark sweeping through N price levels
static void BM_SweepLevels(benchmark::State& state) {
    for (auto _ : state) {
        OrderBook book;
        for (int i = 0; i < state.range(0); i++)
            book.addOrder(Order(i, Side::SELL, 100 + i, 10, i + 1));
        book.addOrder(Order(9999, Side::BUY, 100 + state.range(0), 9999, 9999));
    }
}
BENCHMARK(BM_SweepLevels)->Range(8, 1024);

BENCHMARK_MAIN();
