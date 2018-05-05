#pragma once
#ifndef USI_OPTIONS_HPP
#define USI_OPTIONS_HPP

#include"types.hpp"
#include<algorithm>
#include<string>
#include<unordered_map>

class USIOption{
public:
	long long byoyomi_margin;
	unsigned int random_turn;
	Depth fixed_depth;
    long long USI_Hash;
    unsigned int thread_num;
    bool do_use_mate_search;
};

extern USIOption usi_option;

#endif
