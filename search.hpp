#ifndef SEARCH_HPP
#define SEARCH_HPP

#include"position.hpp"
#include"move.hpp"
#include"hash_table.hpp"
#include<chrono>

const int DEPTH_MAX = 64;
const int MATE_DEPTH_MAX = 9;
const int DEFAULT_TIME_MARGIN = 800;
const int INFINITE = 9999999;

class Searcher {
public:
	Searcher() : limit_msec_(0), node_number_(0), start_(std::chrono::steady_clock::now()), byoyomi_margin_(DEFAULT_TIME_MARGIN) {}
	Searcher(long long limit_msec) : limit_msec_(limit_msec), node_number_(0), start_(std::chrono::steady_clock::now()), byoyomi_margin_(DEFAULT_TIME_MARGIN) {}

	//USIクラスから呼び出される一番大本となる関数
	void think(Position &pos, long long limit);
	void think_fixed_depth(Position &pos, unsigned int fixed_depth);

	//通常探索
	int NegaAlphaBeta(Position &pos, int alpha, int beta, int depth, int depthMax);

	//ルートノードだけNegaScout
	void NegaScout(Position &root, std::vector<Move> &root_moves);

	//静止探索:今は直前に動いた駒を取るだけ
	int qsearch(Position &pos, int alpha, int beta, int depth);

	//GUIとのやりとり類.Seacherが持つべきものではない気がする……
    void sendMove(const Move move);
	void sendInfo(long long time, int depth, int seldepth, long long node_number, std::string cp_or_mate, int score, std::vector<Move> pv);
	void sendInfo(Position k, int depth, std::string cp_or_mate, int score);
	void sendResign(){ std::cout << "bestmove resign" << std::endl; }
	void sendBestMove(const Move bestMove) { std::cout << "bestmove "; sendMove(bestMove); }
	void sendPV(std::vector<Move> pv) { for (Move move : pv) sendMove(move); }

	//pvを作る.これもSeacherが持つべき関数なんだろうか
	std::vector<Move> makePV(Position pos);
	//best_moves_のほうからPV作る関数
	std::vector<Move> pv();

	//時間を見る.TimeManagerのclassを作るべきなのかはよくわからない.
	bool isTimeOver();

	//ラッパー(わかりやすくなってるか?)
	int NullWindowSearch(Position &now, int alpha, int depth, int depth_max) { return NegaAlphaBeta(now, alpha, alpha + 1, depth, depth_max); }
	
	//これもなんか変な気がする
	void setByoyomiMargin(long long msec) { byoyomi_margin_ = msec; }
	void setRandomTurn(unsigned int random_turn) { random_turn_ = random_turn; }
private:
    //制限時間
    long long limit_msec_;

    //探索局面数
    long long node_number_;

	//読み筋
	Move best_moves_[DEPTH_MAX][DEPTH_MAX];

	//置換表
	HashTable hash_table_;

    //時間
    std::chrono::steady_clock::time_point start_;

	//秒読みマージン(やっぱりこれSeacherが持つのおかしい気がしている)
	long long byoyomi_margin_;

	//ランダムに指す手数
	unsigned int random_turn_;
};


#endif
