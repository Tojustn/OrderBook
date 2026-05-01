#pragma once
#include "types.hpp"
#include "price_level.hpp"
class PriceLevelPool{
    public:
        PriceLevelPool() = default;
        explicit PriceLevelPool(size_t capacity);
        PriceLevel* allocate();
        void deallocate(PriceLevel*);
        ~PriceLevelPool();
    private:
        PriceLevel* head_ = nullptr;
};