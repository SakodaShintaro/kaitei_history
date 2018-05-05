#pragma once
#ifndef HISTORY_HPP
#define HISTORY_HPP

#include"piece.hpp"
#include"square.hpp"
#include"move.hpp"
#include<queue>

class History {
private:
    Score table_[PieceNum][SquareNum];
public:
    void updateBetaCutMove(const Move move, const Depth depth) {
        //std::cout << "â¡ì_(depth = "<< depth / PLY << " :";
        //move.print();
        table_[move.subject()][move.to()] += static_cast<int>(depth / PLY) * depth / PLY;
    }

    void updateNonBetaCutMove(const Move move, const Depth depth) {
        //std::cout << "å∏ì_(depth = " << depth / PLY << " :";
        //move.print();
        table_[move.subject()][move.to()] -= static_cast<int>(depth / PLY) * depth / PLY;
    }

    Score operator[](const Move& move) const {
        return table_[move.subject()][move.to()];
    }

    void print() {
        std::priority_queue <std::pair<int, std::pair<int, int>>> pq;
        
        for (int rank = Rank1; rank <= Rank9; rank++) {
            for (int file = File9; file >= File1; file--) {
                int sum = 0;
                for (Piece p : PieceList) {
                    sum += table_[p][FRToSquare[file][rank]];
                    pq.push({ table_[p][FRToSquare[file][rank]], {p, FRToSquare[file][rank]} });
                }
                printf("%6d ", sum);
            }
            printf("\n");
        }

        printf("è„à 10å¬\n");
        for (int i = 0; i < 10; i++) {
            auto p = pq.top();
            pq.pop();
            std::cout << Piece(p.second.first) << " " <<  Square(p.second.second) << " " <<  p.first << std::endl;
        }
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