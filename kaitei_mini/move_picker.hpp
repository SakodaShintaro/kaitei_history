#pragma once

#include"move.hpp"
#include<vector>

class Position;
class History;
extern const int MAX_MOVE_LIST_SIZE;

class MovePicker {
private:
	Move* begin() { return moves_; }
    Move* end() { return end_; }

	const Position& pos_;

	Depth depth_;
	Move tt_move_;

	//指し手生成の段階
	int stage_;

	//次に返す手、生成された指し手の末尾
    Move *cur_, *end_;

    //配列を確保する用のポインタ
    Move *moves_;

    const History& history_;

    Square to_;

public:
	//通常探索から呼ばれる際のコンストラクタ
	MovePicker(const Position& pos, const Move ttMove, const Depth depth, const History& history);

    //静止探索から呼ばれる際のコンストラクタ
    MovePicker(const Position& pos, const Move ttMove, const Depth depth, const History& history, const Square to);

    ~MovePicker() {
        delete[] moves_;
    }
	Move nextMove();
	void generateNextStage();
    void scoreCapture();
    void scoreNonCapture();

    void printAllMoves() {
        for (auto itr = cur_; itr != end_; itr++) {
            itr->printWithScore();
        }
    }
};

extern int FromToHistory[SquareNum][SquareNum];
extern int PieceToHistory;