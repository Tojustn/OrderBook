#pragma once
#include <cstdint>
#include <chrono>

using Price = int64_t; //price in USD cents
using Quantity = int64_t; 
using OrderId = uint64_t;
using UserId = uint64_t;
enum class Side{
    BUY,
    SELL
};

// Match Order would return this, to prevent self trades and also if not a self trade still contains the order
struct MatchResult{
    Quantity remaining;
    bool stpTriggered;
};

enum class AddResult{
    STP_CANCELLED, // Self Trade Prevention
    ADDED, 
    FILLED
};