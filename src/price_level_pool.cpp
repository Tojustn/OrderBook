#include "price_level_pool.hpp"

// Add pricelevel to the pool
PriceLevel* PriceLevelPool::allocate(){
    if(head_){
        PriceLevel* slot = head_;
        head_ = head_->next_;
        *slot = PriceLevel{};
        return slot;
    }
    return new PriceLevel();
};

// Add pricelevel to the pool
void PriceLevelPool::deallocate(PriceLevel* level){
    level->next_ = head_; 
    head_ = level;
};

PriceLevelPool::~PriceLevelPool(){
    while(head_){
        PriceLevel* next = head_->next_;
        delete head_;
        head_ = next;
    }
}
