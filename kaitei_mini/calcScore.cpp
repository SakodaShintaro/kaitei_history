#include"position.hpp"
#include"piece.hpp"
#include"common.hpp"
#include<iostream>
#include<fstream>

enum {
	PAWN_VALUE = 100,
	LANCE_VALUE = 267,
	KNIGHT_VALUE = 295,
	SILVER_VALUE = 424,
	GOLD_VALUE = 510,
	BISHOP_VALUE = 654,
	ROOK_VALUE = 738,
	PAWN_PROMOTE_VALUE = 614,
	LANCE_PROMOTE_VALUE = 562,
	KNIGHT_PROMOTE_VALUE = 586,
	SILVER_PROMOTE_VALUE = 569,
	BISHOP_PROMOTE_VALUE = 951,
	ROOK_PROMOTE_VALUE = 1086,
};

int piece_value[] = {
	0, static_cast<int>(PAWN_VALUE * 1.05), static_cast<int>(LANCE_VALUE * 1.05), static_cast<int>(KNIGHT_VALUE * 1.05), static_cast<int>(SILVER_VALUE * 1.05),
	static_cast<int>(GOLD_VALUE * 1.05), static_cast<int>(BISHOP_VALUE * 1.05), static_cast<int>(ROOK_VALUE * 1.05), 0, 0, //0~9
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, //10~19
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, //20~29
	0, 0, 0, PAWN_VALUE, LANCE_VALUE, KNIGHT_VALUE, SILVER_VALUE, GOLD_VALUE, BISHOP_VALUE, ROOK_VALUE, //30~39
	0, 0, 0, 0, 0, 0, 0, 0, 0, PAWN_PROMOTE_VALUE, //40~49
	LANCE_PROMOTE_VALUE, KNIGHT_PROMOTE_VALUE,  SILVER_PROMOTE_VALUE, 0, BISHOP_PROMOTE_VALUE, ROOK_PROMOTE_VALUE, 0, 0, 0, 0, //50~59
	0, 0, 0, 0, 0, -PAWN_VALUE, -LANCE_VALUE, -KNIGHT_VALUE, -SILVER_VALUE, -GOLD_VALUE, //60~69
	-BISHOP_VALUE, -ROOK_VALUE, 0, 0, 0, 0, 0, 0, 0, 0, //70~79
	0, -PAWN_PROMOTE_VALUE, -LANCE_PROMOTE_VALUE, -KNIGHT_PROMOTE_VALUE, -SILVER_PROMOTE_VALUE, 0, -BISHOP_PROMOTE_VALUE, -ROOK_PROMOTE_VALUE, 0, 0 //80~89
};

Score Position::initScore() {
	//定数
	const int COEFFICIENT = 1;

	piece_score_ = Score(0), control_score_ = Score(0), control_num_score_ = Score(0);

	//盤上にある駒の価値
	for (Square sq : SquareList) {
		if (0 > board_[sq] || PieceNum < board_[sq]) {
			print();
			printHistory();
			assert(false);
		}
		piece_score_ += piece_value[board_[sq]];
		control_num_score_ += control_num_[BLACK][sq];
		control_num_score_ -= control_num_[WHITE][sq];
	}

	//持ち駒の価値
	for (Piece p = PAWN; p <= ROOK; p++) {
		piece_score_ += piece_value[p] * hand_[BLACK].num(p);
		piece_score_ -= piece_value[p] * hand_[WHITE].num(p);
	}

	//玉周辺の利き
	for (int i = 0; i < 8; i++) {
		if (isOnBoard(king_sq_[BLACK] + DirList[i]))
			control_score_ += COEFFICIENT * (control_num_[BLACK][king_sq_[BLACK] + DirList[i]] - control_num_[WHITE][king_sq_[BLACK] + DirList[i]]);
		if (isOnBoard(king_sq_[WHITE] + DirList[i]))
			control_score_ += COEFFICIENT * (control_num_[BLACK][king_sq_[WHITE] + DirList[i]] - control_num_[WHITE][king_sq_[WHITE] + DirList[i]]);
	}
	//玉の位置への利きも足す
	control_score_ += COEFFICIENT * (control_num_[BLACK][king_sq_[BLACK]] - control_num_[WHITE][king_sq_[BLACK]]);
	control_score_ += COEFFICIENT * (control_num_[BLACK][king_sq_[WHITE]] - control_num_[WHITE][king_sq_[WHITE]]);

    return score_ = piece_score_ + control_num_score_ + control_score_;
}

void Position::testPieceValue() {
	std::cout << "HAND_PAWN_VALUE : " << piece_value[PAWN] << std::endl;
	std::cout << "HAND_LANCE_VALUE : " << piece_value[LANCE] << std::endl;
	std::cout << "HAND_KNIGHT_VALUE : " << piece_value[KNIGHT] << std::endl;
	std::cout << "HAND_SILVER_VALUE : " << piece_value[SILVER] << std::endl;
	std::cout << "HAND_GOLD_VALUE : " << piece_value[GOLD] << std::endl;
	std::cout << "HAND_BISHOP_VALUE : " << piece_value[BISHOP] << std::endl;
	std::cout << "HAND_ROOK_VALUE : " << piece_value[ROOK] << std::endl;

	std::cout << "BLACK_PAWN_VALUE : " << piece_value[BLACK_PAWN] << std::endl;
	std::cout << "BLACK_LANCE_VALUE : " << piece_value[BLACK_LANCE] << std::endl;
	std::cout << "BLACK_KNIGHT_VALUE : " << piece_value[BLACK_KNIGHT] << std::endl;
	std::cout << "BLACK_SILVER_VALUE : " << piece_value[BLACK_SILVER] << std::endl;
	std::cout << "BLACK_GOLD_VALUE : " << piece_value[BLACK_GOLD] << std::endl;
	std::cout << "BLACK_BISHOP_VALUE : " << piece_value[BLACK_BISHOP] << std::endl;
	std::cout << "BLACK_ROOK_VALUE : " << piece_value[BLACK_ROOK] << std::endl;
	std::cout << "BLACK_KING_VALUE : " << piece_value[BLACK_KING] << std::endl;

	std::cout << "BLACK_PAWN_PROMOTE_VALUE : " << piece_value[BLACK_PAWN_PROMOTE] << std::endl;
	std::cout << "BLACK_LANCE_PROMOTE_VALUE : " << piece_value[BLACK_LANCE_PROMOTE] << std::endl;
	std::cout << "BLACK_KNIGHT_PROMOTE_VALUE : " << piece_value[BLACK_KNIGHT_PROMOTE] << std::endl;
	std::cout << "BLACK_SILVER_PROMOTE_VALUE : " << piece_value[BLACK_SILVER_PROMOTE] << std::endl;
	std::cout << "BLACK_BISHOP_PROMOTE_VALUE : " << piece_value[BLACK_BISHOP_PROMOTE] << std::endl;
	std::cout << "BLACK_ROOK_PROMOTE_VALUE : " << piece_value[BLACK_ROOK_PROMOTE] << std::endl;

	std::cout << "WHITE_PAWN_VALUE : " << piece_value[WHITE_PAWN] << std::endl;
	std::cout << "WHITE_LANCE_VALUE : " << piece_value[WHITE_LANCE] << std::endl;
	std::cout << "WHITE_KNIGHT_VALUE : " << piece_value[WHITE_KNIGHT] << std::endl;
	std::cout << "WHITE_SILVER_VALUE : " << piece_value[WHITE_SILVER] << std::endl;
	std::cout << "WHITE_GOLD_VALUE : " << piece_value[WHITE_GOLD] << std::endl;
	std::cout << "WHITE_BISHOP_VALUE : " << piece_value[WHITE_BISHOP] << std::endl;
	std::cout << "WHITE_ROOK_VALUE : " << piece_value[WHITE_ROOK] << std::endl;
	std::cout << "WHITE_KING_VALUE : " << piece_value[WHITE_KING] << std::endl;

	std::cout << "WHITE_PAWN_PROMOTE_VALUE : " << piece_value[WHITE_PAWN_PROMOTE] << std::endl;
	std::cout << "WHITE_LANCE_PROMOTE_VALUE : " << piece_value[WHITE_LANCE_PROMOTE] << std::endl;
	std::cout << "WHITE_KNIGHT_PROMOTE_VALUE : " << piece_value[WHITE_KNIGHT_PROMOTE] << std::endl;
	std::cout << "WHITE_SILVER_PROMOTE_VALUE : " << piece_value[WHITE_SILVER_PROMOTE] << std::endl;
	std::cout << "WHITE_BISHOP_PROMOTE_VALUE : " << piece_value[WHITE_BISHOP_PROMOTE] << std::endl;
	std::cout << "WHITE_ROOK_PROMOTE_VALUE : " << piece_value[WHITE_ROOK_PROMOTE] << std::endl;
}

Score Position::score() {
    //他二つはdoMove内で差分計算されているのでcontrol_score_だけを計算して合計を返す
    control_score_ = Score(0);
    //玉周辺の利き
    for (int i = 0; i < 8; i++) {
        if (isOnBoard(king_sq_[BLACK] + DirList[i]))
            control_score_ += (control_num_[BLACK][king_sq_[BLACK] + DirList[i]] - control_num_[WHITE][king_sq_[BLACK] + DirList[i]]);
        if (isOnBoard(king_sq_[WHITE] + DirList[i]))
            control_score_ += (control_num_[BLACK][king_sq_[WHITE] + DirList[i]] - control_num_[WHITE][king_sq_[WHITE] + DirList[i]]);
    }
    //玉の位置への利きも足す
    control_score_ += (control_num_[BLACK][king_sq_[BLACK]] - control_num_[WHITE][king_sq_[BLACK]]);
    control_score_ += (control_num_[BLACK][king_sq_[WHITE]] - control_num_[WHITE][king_sq_[WHITE]]);

    return piece_score_ + control_score_ + control_num_score_;
}