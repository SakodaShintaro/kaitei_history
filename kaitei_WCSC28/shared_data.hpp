#pragma once
#ifndef SHARED_DATA_HPP
#define SHARED_DATA_HPP

#include"hash_table.hpp"
#include<atomic>

struct SharedData {
    std::atomic<bool> stop_signal;
    HashTable hash_table;
    Position root;
    long long limit_msec;
};

extern SharedData shared_data;

#endif // !SHARED_DATA_HPP