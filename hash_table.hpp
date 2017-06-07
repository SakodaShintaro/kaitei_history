#ifndef HASH_TABLE_HPP
#define HASH_TABLE_HPP

#include"hash_entry.hpp"
#include<vector>

class Move;

class HashTable {
public:
    HashTable() { setSize(256); }
    HashEntry* find(long long key);
    void save(long long key, Move move, int depth);
    void setSize(int megabytes);
    double hashfull() { return static_cast<double>(hashfull_) / static_cast<double>(size_) * 1000.0; }
    void clear() { table_.clear(); setSize(256); }
private:
    //ハッシュエントリのvector:これが本体
    std::vector<HashEntry> table_;

    //ハッシュテーブルの要素数
    //これtable.size()では?
    //上位ソフトだといくつかのエントリをまとめて一つにしてるからこれが別に必要なんだろうか
    size_t size_;

    //ハッシュキーからテーブルのインデックスを求めるためのマスク
    size_t key_mask_;

    //使用済みエントリの数
    size_t hashfull_;

    //ハッシュテーブルに入っている情報の古さ
    uint8_t age_;
};

#endif
