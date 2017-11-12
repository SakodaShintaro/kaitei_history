#ifndef HASH_TABLE_HPP
#define HASH_TABLE_HPP

#include"hash_entry.hpp"
#include<vector>
#include<mutex>

class Move;

class HashTable{
public:
    HashTable() { setSize(64); }
	HashEntry* find(long long key);
	void save(long long key, Move move, Score score, Depth depth);
	void setSize(long long megabytes);
	double hashfull() { return static_cast<double>(hashfull_) / static_cast<double>(size_) * 1000.0; }
	void clear() { 
        table_.clear();
    }
private:
    //ハッシュエントリのvector:これが本体
    std::vector<HashEntry> table_;

    //ハッシュテーブルの要素数
	//これtable.size()では?
	//上位ソフトだといくつかのエントリをまとめて一つにしてるからこれが別に必要なんだろうか
    //それにしてもそのまとめた数さえ保持しておけばいいんじゃないのか
    size_t size_;

    //ハッシュキーからテーブルのインデックスを求めるためのマスク
    size_t key_mask_;

    //使用済みエントリの数
    size_t hashfull_;

    //ハッシュテーブルに入っている情報の古さ
    uint8_t age_;
};

#endif