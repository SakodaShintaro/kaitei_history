#pragma once
#ifndef EVAL_ELEMENTS_HPP
#define EVAL_ELEMENTS_HPP

#include"piece.hpp"
#include"piece_state.hpp"
#include<algorithm>

constexpr int PIECE_STATE_LIST_SIZE = 38;

//評価値の計算に用いる特徴量をまとめたもの
struct Features {
	//玉位置がint型なのは現状まだSquare型は壁(番兵)分があって数字とマスがうまく対応していないから
	int black_king_sq; //先手玉の位置
	int white_king_sq_reversed; //後手玉の位置を反転させていれる
    PieceState piece_state_list[PIECE_STATE_LIST_SIZE];

    inline bool operator==(const Features& rhs) {
        return (black_king_sq == rhs.black_king_sq
            && white_king_sq_reversed == rhs.white_king_sq_reversed
            && piece_state_list == rhs.piece_state_list);
    }
    inline bool operator!=(const Features& rhs) {
        return !(*this == rhs);
    }
};

inline bool isEqual(Features f1, Features f2) {
    //ソートしてpiece_state_listの中身さえ合っていればいいという比較関数
    std::sort(f1.piece_state_list, f1.piece_state_list + PIECE_STATE_LIST_SIZE);
    std::sort(f2.piece_state_list, f2.piece_state_list + PIECE_STATE_LIST_SIZE);
    return (f1.black_king_sq == f1.black_king_sq && f2.white_king_sq_reversed == f2.white_king_sq_reversed && f1.piece_state_list == f2.piece_state_list);
}

inline std::ostream& operator<<(std::ostream& os, Features f) {
    os << "先手玉:" << f.black_king_sq << std::endl;
    os << "後手玉:" << f.white_king_sq_reversed << std::endl;
    for (auto e : f.piece_state_list) {
        os << e << std::endl;
    }
    return os;
}

#endif // !EVAL_ELEMENT_HPP
