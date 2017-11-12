#ifndef HASH_ENTRY_HPP
#define HASH_ENTRY_HPP

#include"move.hpp"

struct HashEntry{
	//ハッシュの値
    long long hash_val_;

	//その局面における(仮の)最善手
    Move best_move_;

	//残り探索深さ
    Depth depth_;

    //探索による評価値
    Score score_;

	//登録されているか
	bool flag_;

    HashEntry() :
        hash_val_(0),
        best_move_(NULL_MOVE),
        depth_(Depth(0)),
        score_(Score(0)),
        flag_(false) {}
};

#endif