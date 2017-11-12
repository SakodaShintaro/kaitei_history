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
    long long USI_Hash;
};

extern USIOption usi_option;

#endif