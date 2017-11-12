#ifndef SEARCH_HPP
#define SEARCH_HPP

#include"position.hpp"
#include"move.hpp"
#include"hash_table.hpp"
#include"usi_options.hpp"
#include"shared_data.hpp"
#include"pv_table.hpp"
#include"history.hpp"
#include<chrono>

class Searcher {
public:
	Searcher(bool is_main_thread) : node_number_(0), start_(std::chrono::steady_clock::now()), is_main_thread_(is_main_thread) {}

	void think();
	
    //現状NegaAlphaBetaより遅い:なぜだろう
    template<bool isPV>
    Score search(Position &pos, Score alpha, Score beta, Depth depth, int distance_from_root);

	//詰み探索:今は使っていない
	void search_mate(Position pos);
    bool search_check(Position &pos, int depth, int depth_max);
    bool search_evasion(Position &pos, int depth, int depth_max);

	//静止探索:今は直前に動いた駒を取るだけ
    template<bool isPV>
	Score qsearch(Position &pos, Score alpha, Score beta, Depth depth, int distance_from_root);

	//GUIへの情報
    void sendInfo(Depth depth, std::string cp_or_mate, Score score, Bound bound);

    inline void updatePV(Move* pv, Move move, Move* childPV) {
        //一個下のノードのPVをコピーする
        int counter = 0;
        for (*pv++ = move; childPV != nullptr && *childPV != NULL_MOVE; ++childPV) {
            *pv++ = *childPV;
            if (++counter >= 10) {
                assert(false);
            }
        }
        //末尾にはNULL_MOVEを入れる
        *pv = NULL_MOVE;
    }

	//時間を見る.TimeManagerのclassを作るべきなのかはよくわからない.
    inline bool isTimeOver() {
        auto now_time = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now_time - start_);
        return (elapsed.count() >= shared_data.limit_msec - usi_option.byoyomi_margin);
    }

    void setMainThread(bool b) {
        is_main_thread_ = b;
    }
    
    inline bool shouldStop() {
        if (is_main_thread_) { //MainThreadなら時間の確認と停止信号の確認
            if (isTimeOver() || shared_data.stop_signal) {
                //停止信号をオンにする
                shared_data.stop_signal = true;
                return true;
            }
        } else { //SlaveThreadなら停止信号だけを見る
            if (shared_data.stop_signal)
                return true;
        }
        return false;
    }

	inline static int razorMargin(int depth) { return 256 + 32 * depth; }
	inline static int futilityMargin(int depth, double progress) { return static_cast<int>((120 + 80 * progress) * depth); }
	inline static int depthMargin(int depth) { return 126 + 32 * depth; }
private:
    //探索局面数
    long long node_number_;

    //時間
    std::chrono::steady_clock::time_point start_;

    //MainThreadかどうか
    bool is_main_thread_;

    //History 
    History history_;

    //Seldpth
    Depth seldepth_;

    PVTable pv_;
};

#endif