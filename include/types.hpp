#pragma once
#include <cstdint>
#include <chrono>

using Price = int64_t; //price in USD cents
using Quantity = int64_t; 
using OrderId = uint64_t;

enum Side{
    BUY,
    SELL
};