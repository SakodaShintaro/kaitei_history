#include "hash_table.hpp"
#include "common.hpp"
#include <iostream>

HashEntry* HashTable::find(long long key)
{
    if (table_[key & key_mask_].flag_ == false) return NULL;
    else if (table_[key & key_mask_].hash_val_ != key) return NULL;
    else return &table_[key & key_mask_];
}

void HashTable::save(long long key, Move move, int depth)
{
    HashEntry* target = &table_[key & key_mask_];
    target->hash_val_ = key;
    target->best_move_ = move;
    target->remain_depth_ = depth;
    if (!target->flag_) {
        target->flag_ = true;
        hashfull_++;
    }
}

void HashTable::setSize(int megabytes)
{
    int bytes = megabytes * 1024 * 1024;
    size_ = highestOneBit(bytes / sizeof(HashEntry));
    age_ = 0;
    key_mask_ = size_ - 1;
    hashfull_ = 0;
    age_ = 0;
    table_.resize(size_);
}
