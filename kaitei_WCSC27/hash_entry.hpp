#ifndef HASH_ENTRY_HPP
#define HASH_ENTRY_HPP

#include"move.hpp"

class HashEntry {
public:
    //ハッシュの値
    long long hash_val_;

    //その局面における(仮の)最善手
    Move best_move_;

    //残り探索深さ
    short remain_depth_;

    //登録されているか
    bool flag_;
};

#endif
