#pragma once
#ifndef HISTORY_HPP
#define HISTORY_HPP

#include"piece.hpp"
#include"square.hpp"
#include"move.hpp"

class History {
private:
    Score table_[PieceNum][SquareNum];
public:
    void updateBetaCutMove(const Move move, const Depth depth) {
        table_[move.subject()][move.to()] += static_cast<int>(depth) * depth;
    }

    void updateNonBetaCutMove(const Move move, const Depth depth) {
        table_[move.subject()][move.to()] -= static_cast<int>(depth) * depth;
    }

    Score operator[](const Move& move) const {
        return table_[move.subject()][move.to()];
    }

    void clear() {
        for (int piece = 0; piece < PieceNum; ++piece) {
            for (int to = 0; to < SquareNum; ++to) {
                table_[piece][to] = SCORE_ZERO;
            }
        }
    }
};


#endif // !HISTORY_HPP