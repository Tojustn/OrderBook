#include <chrono>
#include <cstdint>
#include <cstdio>
#include <fstream>
#include <iostream>
#include <string>
#include <string_view>
#include <unordered_set>
#include "order_book.hpp"
#include "order.hpp"
#include "types.hpp"

#ifdef _MSC_VER
#  include <intrin.h>
static inline uint64_t rdtsc() { return __rdtsc(); }
#else
static inline uint64_t rdtsc() {
    unsigned int lo, hi;
    __asm__ __volatile__("rdtsc" : "=a"(lo), "=d"(hi));
    return (static_cast<uint64_t>(hi) << 32) | lo;
}
#endif

// Calibrate by measuring cycles over a 100ms wall-clock window
static double ns_per_cycle() {
    using clk = std::chrono::steady_clock;
    const auto     wall0 = clk::now();
    const uint64_t cyc0  = rdtsc();
    while (std::chrono::duration<double>(clk::now() - wall0).count() < 0.1) {}
    const uint64_t cyc1  = rdtsc();
    const auto     wall1 = clk::now();
    const double   ns    = std::chrono::duration<double, std::nano>(wall1 - wall0).count();
    return ns / static_cast<double>(cyc1 - cyc0);
}

// output file data record
// event_idx, op_type, side, latency_ns

// This file is meant to take the data from events.csv -> latency.csv
int main(){

    const double ns = ns_per_cycle();

    // Note that the first 100k events will be used to warm up orderbook
    OrderBook orderBook{};

    // input stream
    std::ifstream inFile("data/events.csv");

    // output stream
    std::ofstream outFile("data/latency.csv");

    if(!inFile.is_open()){
        std::cerr << "Couldn't open file\n";
        return 1;
    }

    outFile << "event_idx,op_type,latency_ns\n";

    std::string line;
    std::getline(inFile, line); // skip header

    uint64_t event_idx{0};

    std::unordered_set<uint64_t> levelSet;

    while (std::getline(inFile, line)) {
        Side    side{};
        char    op{};
        int64_t price{};
        int64_t size{};
        size_t  start_idx{0};
        uint8_t cur_column{0};

        for (size_t i = 0; i <= line.size(); ++i) {
            if (i == line.size() || line[i] == ',') {
                std::string_view token(line.data() + start_idx, i - start_idx);

                switch (cur_column) {
                    case 0: break; // ts — unused
                    case 1: side  = (token[0] == 'B') ? Side::BUY : Side::SELL; break;
                    case 2: op    = token[0]; break;
                    case 3: price = static_cast<int64_t>(std::stod(std::string(token)) * 10); break;
                    case 4: size  = static_cast<int64_t>(std::stod(std::string(token)) * 1'000'000); break;
                }

                ++cur_column;
                start_idx = i + 1;
            }
        }

        UserId userId = (side == Side::BUY) ? 1 : 2;
        uint64_t side_bit = (side == Side::SELL) ? 1 : 0;
        // Shift the bit down one and then use a bitwise or (last bit will be either 1 or 0 ) the last digit is the identifier
        uint64_t key = (static_cast<uint64_t>(price) << 1) | side_bit;

        uint64_t t0{0};
        uint64_t t1{0};
        double time_diff{0};
        if (op == 'A') {
            if (levelSet.find(key) == levelSet.end()) {
                t0 = rdtsc();
                orderBook.addOrder(Order{key, side, price, size, userId});
                t1 = rdtsc();
                levelSet.insert(key);
            } else {
                op = 'M';
                t0 = rdtsc();
                orderBook.modifyOrder(key, size);
                t1 = rdtsc();
            }
        } else {

            t0 = rdtsc();
            auto result = orderBook.cancelOrder(key);
            t1 = rdtsc();
            if (result == CancelResult::CANCELLED) {
                levelSet.erase(key);
            }
        }
        time_diff = (t1-t0) * ns;
        outFile << event_idx << ',' << op << ',' << time_diff << '\n';
    

        ++event_idx;
    }

}