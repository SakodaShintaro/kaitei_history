#ifndef SEARCH_HPP
#define SEARCH_HPP

#include"position.hpp"
#include"move.hpp"
#include"hash_table.hpp"
#include"usi_options.hpp"
#include"shared_data.hpp"
#include"search_stack.hpp"
#include"pv_table.hpp"
#include<chrono>

class Searcher {
public:
    //役割
    enum Role {
        MAIN, SLAVE, MATE
    };

	Searcher(Role r) : node_number_(0), start_(std::chrono::steady_clock::now()), role_(r) {}

	void think();
    Move thinkForGenerateLearnData(Position &root, Depth fixed_depth, unsigned int random_turn);

	//単純なAlphaBeta.学習に使う
    template<bool isPVNode>
	Score NegaAlphaBeta(Position &pos, Score alpha, Score beta, Depth depth, int distance_from_root);
	
    //探索で用いる関数
    template<bool isPVNode>
    Score search(Position &pos, Score alpha, Score beta, Depth depth, int distance_from_root);

	//詰み探索:今は使っていない
	void search_mate(Position& root);
    bool search_check(Position &pos, Depth depth);
    bool search_evasion(Position &pos, Depth depth);

	//静止探索
    template<bool isPVNode>
	Score qsearch(Position &pos, Score alpha, Score beta, Depth depth, int distance_from_root);

	//GUIへの情報
    void sendInfo(Depth depth, std::string cp_or_mate, Score score, Bound bound);

	//時間を見る.TimeManagerのclassを作るべきなのかはよくわからない.
    inline bool isTimeOver() {
        auto now_time = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now_time - start_);
        return (elapsed.count() >= shared_data.limit_msec - usi_option.byoyomi_margin);
    }

    void setRole(Role r) {
        role_ = r;
    }
    
    inline bool shouldStop() {
        //TODO : 探索深さの制限も加えるべき
        if (role_ == MAIN) { //MainThreadなら時間の確認と停止信号の確認
            if (isTimeOver() || shared_data.stop_signal) {
                //停止信号をオンにする
                shared_data.stop_signal = true;
                return true;
            }
        } else { //SlaveThreadなら停止信号だけを見る
            if (shared_data.stop_signal) {
                return true;
            }
        }
        return false;
    }

    SearchStack* searchInfoAt(int distance_from_root) {
        //深さ0でも前二つが参照できるようにずらしておかなければならない
        return &stack_[distance_from_root + 2];
    }

    std::vector<Move> pv() {
        std::vector<Move> pv;
        for (Move m : pv_table_)
            pv.push_back(m);

#if DEBUG
        Position pos = root_;
        for (Move m : pv) {
            if (pos.isLegalMove(m)) {
                pos.doMove(m);
            } else {
                std::cout << "root" << std::endl;
                root_.print();
                std::cout << "pos" << std::endl;
                pos.print();
                std::cout << "pv" << std::endl;
                for (Move s : pv) {
                    s.print();
                }
                assert(false);
            }
        }
#endif // DEBUG

        return pv;
    }

    void setRoot(const Position& pos) {
        root_ = pos;
        root_moves_ = pos.generateAllMoves();
    }

    void resetPVTable() {
        pv_table_.reset();
    }

	inline static int depthMargin(int depth) { return 64 + 32 * depth / PLY; }

private:
    //探索局面数
    long long node_number_;

    //時間
    std::chrono::steady_clock::time_point start_;

    //役割
    Role role_;

    //History 
    History history_;

    //root_moves_は持っておいた方が良さそう
    std::vector<Move> root_moves_;

    //Seldpth
    Depth seldepth_;

    PVTable pv_table_;

    SearchStack stack_[DEPTH_MAX];

    Position root_;
};

#endif