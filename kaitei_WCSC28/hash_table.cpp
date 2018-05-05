#include "hash_table.hpp"
#include "common.hpp"
#include <iostream>

HashEntry* HashTable::find(long long key) {
	if (table_[key & key_mask_].flag_ == false) return nullptr;
	if (table_[key & key_mask_].hash_val_ != key) return nullptr;
	return &table_[key & key_mask_];
}

void HashTable::save(long long key, Move move, Score score, Depth depth) {
	HashEntry* target = &table_[key & key_mask_];
	target->hash_val_ = key;
	target->best_move_ = move;
	target->depth_ = depth;
    target->best_move_.score = score;
	if (!target->flag_) {
		target->flag_ = true;
		hashfull_++;
    }
}

void HashTable::setSize(long long megabytes) {
    long long bytes = megabytes * 1024 * 1024;
	size_ = 1ull << MSB64(bytes / sizeof(HashEntry));
    age_ = 0;
    key_mask_ = size_ - 1;
    hashfull_ = 0;
	table_.resize(size_);
}