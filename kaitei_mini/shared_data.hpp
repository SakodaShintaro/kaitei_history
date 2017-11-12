#pragma once
#ifndef SHARED_DATA_HPP
#define SHARED_DATA_HPP

#include"hash_table.hpp"
#include<atomic>
#include<mutex>

struct SharedData {
    std::atomic<bool> stop_signal;
    HashTable hash_table;
    Position root;
    long long limit_msec;
    std::mutex mutex;
};

extern SharedData shared_data;

#endif // !SHARED_DATA_HPP