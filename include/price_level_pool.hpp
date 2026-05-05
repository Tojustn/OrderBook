#pragma once
#include "types.hpp"
#include "price_level.hpp"
class PriceLevelPool{
    public:
        PriceLevelPool() = default;
        explicit PriceLevelPool(size_t capacity);
        PriceLevel* allocate();
        void deallocate(PriceLevel*) noexcept;
        ~PriceLevelPool();
        PriceLevelPool(const PriceLevelPool&) = delete;
        PriceLevelPool& operator=(const PriceLevelPool&) = delete;
        PriceLevelPool(PriceLevelPool&&) noexcept;
        PriceLevelPool& operator=(PriceLevelPool&&) noexcept;
    private:
        PriceLevel* head_ = nullptr;
};