#include"position.hpp"
#include"piece.hpp"
#include"common.hpp"
#include"piece_state.hpp"
#include"eval_params.hpp"
#include<iostream>
#include<fstream>

const int EVAL_SCALE = 32;

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
	piece_score_ = Score(0);

	//盤上にある駒の価値
	for (Square sq : SquareList) {
		if (0 > board_[sq] || PieceNum < board_[sq]) {
			print();
			printHistory();
			assert(false);
		}
		piece_score_ += piece_value[board_[sq]];
	}

	//持ち駒の価値
	for (Piece p = PAWN; p <= ROOK; p++) {
		piece_score_ += piece_value[p] * hand_[BLACK].num(p);
		piece_score_ -= piece_value[p] * hand_[WHITE].num(p);
	}
    kp_score_ = Score(0);
    pp_score_ = Score(0);

    if (eval_params == nullptr) {
        return Score(0);
    }

    auto ee = makeEvalElements();
    for (unsigned int i = 0; i < PIECE_STATE_LIST_SIZE; i++) {
        kp_score_ += eval_params->kp[ee.black_king_sq][ee.piece_state_list[i]];
        kp_score_ -= eval_params->kp[ee.white_king_sq_reversed][invPieceState(ee.piece_state_list[i])];
        for (unsigned int j = i; j < PIECE_STATE_LIST_SIZE; j++) {
            pp_score_ += eval_params->pp[ee.piece_state_list[i]][ee.piece_state_list[j]];
        }
    }

    return piece_score_;
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

Score Position::score() const {
    return piece_score_ + (kp_score_ + pp_score_) / EVAL_SCALE;
}

Features Position::makeEvalElements() const {
	Features ee;
	ee.black_king_sq = SquareToNum[king_sq_[BLACK]];
	ee.white_king_sq_reversed = SquareToNum[InvSquare[king_sq_[WHITE]]];


    int index = 0;
	for (Square sq : SquareList) {
		if (board_[sq] != EMPTY && kind(board_[sq]) != KING) {
			ee.piece_state_list[index++] = pieceState(board_[sq], sq);
		}
	}
	for (Piece p = PAWN; p <= ROOK; p++) {
        for (Color c : {BLACK, WHITE}) {
            for (int i = 0; i < hand_[c].num(p); i++) {
               	ee.piece_state_list[index++] = pieceState(kind(p), i + 1, c);
            }
        }
	}
    assert(index == PIECE_STATE_LIST_SIZE);
	return ee;
}

Features Position::makeEvalElements(std::vector<Move>& pv, Color& leaf_color) {
	//pvがうまくとれているとは限らないので局面を動かした回数を数えておく
	int counter = 0;
	for (Move move : pv) {
		if (isLegalMove(move)) {
			doMove(move);
			counter++;
        } else {
            break;
        }
	}
    leaf_color = color_;
#if DEBUG
    print();
    printHistory();
#endif
	Features ee = ee_;
	for (int i = 0; i < counter; i++)
		undo();
	return ee;
}

Move Position::transformValidMove(const Move move) {
    //stringToMoveではどっちの手番かがわからない
    //つまりsubjectが完全には入っていないので手番付きの駒を入れる
    if (move.isDrop()) {
        return dropMove(move.to(), (color_ == BLACK ? toBlack(move.subject()) : toWhite(move.subject())));
    } else {
        return Move(move.to(), move.from(), false, move.isPromote(), board_[move.from()], board_[move.to()]);
    }
}

Score Position::calcScoreDiff() {
    Move move = lastMove();
    //移動する駒が玉かどうか、captureかどうかの4通りで場合わけ
    if (kind(move.subject()) == KING) {
        if (move.capture() == EMPTY) { //玉を動かし、駒は取らない手
            if (pieceToColor(move.subject()) == BLACK) { //先手玉を動かす場合
                for (int i = 0; i < PIECE_STATE_LIST_SIZE; i++) {
                    kp_score_ -= eval_params->kp[SquareToNum[move.from()]][ee_.piece_state_list[i]];
                    kp_score_ += eval_params->kp[SquareToNum[move.to()]][ee_.piece_state_list[i]];
                }
            } else { //後手玉を動かす場合
                for (int i = 0; i < PIECE_STATE_LIST_SIZE; i++) {
                    kp_score_ += eval_params->kp[SquareToNum[InvSquare[move.from()]]][invPieceState(ee_.piece_state_list[i])];
                    kp_score_ -= eval_params->kp[SquareToNum[InvSquare[move.to()]]][invPieceState(ee_.piece_state_list[i])];
                }
            }
        } else { //玉を動かし、駒を取る手
            Color move_color = pieceToColor(move.subject());
            if (move_color == BLACK) { //先手玉を動かす場合
                PieceState captured = pieceState(move.capture(), move.to(), ~move_color);
                int change_index = -1;
                //captureされたPについてKP, PPを引く
                for (int i = 0; i < PIECE_STATE_LIST_SIZE; i++) {
                    kp_score_ -= eval_params->kp[SquareToNum[move.from()]][ee_.piece_state_list[i]];
                    pp_score_ -= eval_params->pp[captured][ee_.piece_state_list[i]];
                    if (captured == ee_.piece_state_list[i]) {
                        change_index = i;
                    }
                }
                //敵玉とのKP
                kp_score_ += eval_params->kp[ee_.white_king_sq_reversed][invPieceState(captured)];

                assert(change_index != -1);

                PieceState add = pieceState(kind(move.capture()), hand_[move_color].num(kind(move.capture())), move_color);

                //追加されるPについてKP,PPを足す
                for (int i = 0; i < PIECE_STATE_LIST_SIZE; i++) {
                    if (i == change_index) {
                        ee_.piece_state_list[i] = add;
                    }
                    kp_score_ += eval_params->kp[SquareToNum[move.to()]][ee_.piece_state_list[i]];
                    pp_score_ += eval_params->pp[add][ee_.piece_state_list[i]];
                }
                //敵玉とのKP
                kp_score_ -= eval_params->kp[ee_.white_king_sq_reversed][invPieceState(add)];

            } else { //後手玉を動かす場合
                PieceState captured = pieceState(move.capture(), move.to(), ~move_color);
                int change_index = -1;
                //captureされたPについてKP, PPを引く
                for (int i = 0; i < PIECE_STATE_LIST_SIZE; i++) {
                    kp_score_ += eval_params->kp[SquareToNum[InvSquare[move.from()]]][invPieceState(ee_.piece_state_list[i])];
                    pp_score_ -= eval_params->pp[captured][ee_.piece_state_list[i]];
                    if (captured == ee_.piece_state_list[i]) {
                        change_index = i;
                    }
                }
                //敵玉とのKP
                kp_score_ -= eval_params->kp[ee_.black_king_sq][captured];

                assert(change_index != -1);

                PieceState add = pieceState(kind(move.capture()), hand_[move_color].num(kind(move.capture())), move_color);

                //追加されるPについてKP,PPを足す
                for (int i = 0; i < PIECE_STATE_LIST_SIZE; i++) {
                    if (i == change_index) {
                        ee_.piece_state_list[i] = add;
                    }
                    kp_score_ -= eval_params->kp[SquareToNum[InvSquare[move.to()]]][invPieceState(ee_.piece_state_list[i])];
                    pp_score_ += eval_params->pp[add][ee_.piece_state_list[i]];
                }

                //敵玉とのKP
                kp_score_ += eval_params->kp[ee_.black_king_sq][add];
            }
        }
    } else if (move.isDrop()) { //駒を打つ手
        //変化するのはKP, PP両方
        //打つために消えるKP,PPを引く
        PieceState dropped_from = pieceState(kind(move.subject()), hand_[pieceToColor(move.subject())].num(move.subject()) + 1, pieceToColor(move.subject()));
        int change_index = -1;
        //KP
        kp_score_ -= eval_params->kp[ee_.black_king_sq][dropped_from];
        kp_score_ += eval_params->kp[ee_.white_king_sq_reversed][invPieceState(dropped_from)];
        //PP
        for (int i = 0; i < PIECE_STATE_LIST_SIZE; i++) {
            pp_score_ -= eval_params->pp[dropped_from][ee_.piece_state_list[i]];
            if (dropped_from == ee_.piece_state_list[i]) {
                change_index = i;
            }
        }

        assert(change_index != -1);

        PieceState dropped_to = pieceState(move.subject(), move.to(), pieceToColor(move.subject()));

        //KP
        kp_score_ += eval_params->kp[ee_.black_king_sq][dropped_to];
        kp_score_ -= eval_params->kp[ee_.white_king_sq_reversed][invPieceState(dropped_to)];
        //PP
        for (int i = 0; i < PIECE_STATE_LIST_SIZE; i++) {
            if (i == change_index) {
                ee_.piece_state_list[i] = dropped_to;
            }
            pp_score_ += eval_params->pp[dropped_to][ee_.piece_state_list[i]];
        }
    } else {
        if (move.capture() == EMPTY) {
            //変化するのはKP, PP両方
            //打つために消えるKP,PPを引く
            PieceState removed_from = pieceState(move.subject(), move.from(), pieceToColor(move.subject()));
            int change_index = -1;
            //KP
            kp_score_ -= eval_params->kp[ee_.black_king_sq][removed_from];
            kp_score_ += eval_params->kp[ee_.white_king_sq_reversed][invPieceState(removed_from)];
            //PP
            for (int i = 0; i < PIECE_STATE_LIST_SIZE; i++) {
                pp_score_ -= eval_params->pp[removed_from][ee_.piece_state_list[i]];
                if (removed_from == ee_.piece_state_list[i]) {
                    change_index = i;
                }
            }

            assert(change_index != -1);

            PieceState added_to = pieceState(move.isPromote() ? promote(move.subject()) : move.subject(), move.to(), pieceToColor(move.subject()));

            //KP
            kp_score_ += eval_params->kp[ee_.black_king_sq][added_to];
            kp_score_ -= eval_params->kp[ee_.white_king_sq_reversed][invPieceState(added_to)];
            //PP
            for (int i = 0; i < PIECE_STATE_LIST_SIZE; i++) {
                if (i == change_index) {
                    ee_.piece_state_list[i] = added_to;
                }
                pp_score_ += eval_params->pp[added_to][ee_.piece_state_list[i]];
            }
        } else {
            //2つPが消えて2つPが増える.厄介
            PieceState removed1 = pieceState(move.subject(), move.from(), pieceToColor(move.subject()));
            PieceState removed2 = pieceState(move.capture(), move.to(), pieceToColor(move.capture()));
            int change_index1 = -1, change_index2 = -1;
            //KP
            kp_score_ -= eval_params->kp[ee_.black_king_sq][removed1];
            kp_score_ += eval_params->kp[ee_.white_king_sq_reversed][invPieceState(removed1)];
            kp_score_ -= eval_params->kp[ee_.black_king_sq][removed2];
            kp_score_ += eval_params->kp[ee_.white_king_sq_reversed][invPieceState(removed2)];
            //PP
            for (int i = 0; i < PIECE_STATE_LIST_SIZE; i++) {
                pp_score_ -= eval_params->pp[removed1][ee_.piece_state_list[i]];
                pp_score_ -= eval_params->pp[removed2][ee_.piece_state_list[i]];
                if (removed1 == ee_.piece_state_list[i]) {
                    change_index1 = i;
                }
                if (removed2 == ee_.piece_state_list[i]) {
                    change_index2 = i;
                }
            }
            //同じものを2回引いているので補正する
            pp_score_ += eval_params->pp[removed1][removed2];

            assert(change_index1 != -1 && change_index2 != -1);

            PieceState added1 = pieceState(move.isPromote() ? promote(move.subject()) : move.subject(), move.to(), pieceToColor(move.subject()));
            PieceState added2 = pieceState(kind(move.capture()), hand_[pieceToColor(move.subject())].num(move.capture()), pieceToColor(move.subject()));

            //KP
            kp_score_ += eval_params->kp[ee_.black_king_sq][added1];
            kp_score_ -= eval_params->kp[ee_.white_king_sq_reversed][invPieceState(added1)];
            kp_score_ += eval_params->kp[ee_.black_king_sq][added2];
            kp_score_ -= eval_params->kp[ee_.white_king_sq_reversed][invPieceState(added2)];

            //PP
            for (int i = 0; i < PIECE_STATE_LIST_SIZE; i++) {
                if (i == change_index1) {
                    ee_.piece_state_list[i] = added1;
                }
                if (i == change_index2) {
                    ee_.piece_state_list[i] = added2;
                }

                pp_score_ += eval_params->pp[added1][ee_.piece_state_list[i]];
                pp_score_ += eval_params->pp[added2][ee_.piece_state_list[i]];
            }
            //同じものを2回足しているので補正する
            pp_score_ -= eval_params->pp[added1][added2];
        }
    }

#ifdef DEBUG
    int old_kp = kp_score_, old_pp = pp_score_;
    initScore();
    if (old_kp != kp_score_ || old_pp != pp_score_) {
        print();
        move.print();
        assert(false);
    }
#endif

    return piece_score_ + (kp_score_ + pp_score_) / EVAL_SCALE;
}

void Position::printScoreDetail() const {
    auto ee = makeEvalElements();
    for (unsigned int i = 0; i < PIECE_STATE_LIST_SIZE; i++) {
        std::cout << ee.black_king_sq << " " << ee.piece_state_list[i] << " += " << eval_params->kp[ee.black_king_sq][ee.piece_state_list[i]] << std::endl;
        std::cout << ee.white_king_sq_reversed << " " << invPieceState(ee.piece_state_list[i]) << " -= " << eval_params->kp[ee.white_king_sq_reversed][invPieceState(ee.piece_state_list[i])] << std::endl;
        for (unsigned int j = i; j < PIECE_STATE_LIST_SIZE; j++) {
            std::cout << ee.piece_state_list[i] << " " << ee.piece_state_list[j] << " += " << eval_params->pp[ee.piece_state_list[i]][ee.piece_state_list[j]] << std::endl;
        }
    }
}