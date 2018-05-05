#ifndef HASH_ENTRY_HPP
#define HASH_ENTRY_HPP

#include"move.hpp"

struct HashEntry{
	//ハッシュの値
    long long hash_val_;

	//その局面における(仮の)最善手
    //評価値込み
    Move best_move_;

	//残り探索深さ
    Depth depth_;

	//登録されているか
	bool flag_;

    HashEntry() :
        hash_val_(0),
        best_move_(NULL_MOVE),
        depth_(Depth(-1)),
        flag_(false) {}

    void print() {
        printf("hash_val_ = %llx\n", hash_val_);
        printf("best_move_ = "); best_move_.printWithScore();
        printf("depth_ = %d\n", depth_);
        printf("flag_ = %s\n", flag_ ? "YES" : "NO");
    }
};

#endif