#pragma once
#include"position.hpp"
#include"eval_params.hpp"

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


static int piece_value[] = {
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

//どの評価関数を使うかをインスタンスごとに分けるためのPositionクラス
class PositionForLearn : public Position {
public:
    std::unique_ptr<EvalParams<int>> ptr;
    Score initScore();
    Score calcScoreDiff();
};

Score PositionForLearn::initScore() {
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
        kp_score_ += ptr->kp[ee.black_king_sq][ee.piece_state_list[i]];
        kp_score_ -= ptr->kp[ee.white_king_sq_reversed][invPieceState(ee.piece_state_list[i])];
        for (unsigned int j = i; j < PIECE_STATE_LIST_SIZE; j++) {
            pp_score_ += ptr->pp[ee.piece_state_list[i]][ee.piece_state_list[j]];
        }
    }

    return piece_score_;
}

Score PositionForLearn::calcScoreDiff() {
    Move move = lastMove();
    //移動する駒が玉かどうか、captureかどうかの4通りで場合わけ
    if (kind(move.subject()) == KING) {
        if (move.capture() == EMPTY) { //玉を動かし、駒は取らない手
            if (pieceToColor(move.subject()) == BLACK) { //先手玉を動かす場合
                for (int i = 0; i < PIECE_STATE_LIST_SIZE; i++) {
                    kp_score_ -= ptr->kp[SquareToNum[move.from()]][ee_.piece_state_list[i]];
                    kp_score_ += ptr->kp[SquareToNum[move.to()]][ee_.piece_state_list[i]];
                }
            } else { //後手玉を動かす場合
                for (int i = 0; i < PIECE_STATE_LIST_SIZE; i++) {
                    kp_score_ += ptr->kp[SquareToNum[InvSquare[move.from()]]][invPieceState(ee_.piece_state_list[i])];
                    kp_score_ -= ptr->kp[SquareToNum[InvSquare[move.to()]]][invPieceState(ee_.piece_state_list[i])];
                }
            }
        } else { //玉を動かし、駒を取る手
            Color move_color = pieceToColor(move.subject());
            if (move_color == BLACK) { //先手玉を動かす場合
                PieceState captured = pieceState(move.capture(), move.to(), ~move_color);
                int change_index = -1;
                //captureされたPについてKP, PPを引く
                for (int i = 0; i < PIECE_STATE_LIST_SIZE; i++) {
                    kp_score_ -= ptr->kp[SquareToNum[move.from()]][ee_.piece_state_list[i]];
                    pp_score_ -= ptr->pp[captured][ee_.piece_state_list[i]];
                    if (captured == ee_.piece_state_list[i]) {
                        change_index = i;
                    }
                }
                //敵玉とのKP
                kp_score_ += ptr->kp[ee_.white_king_sq_reversed][invPieceState(captured)];

                assert(change_index != -1);

                PieceState add = pieceState(kind(move.capture()), hand_[move_color].num(kind(move.capture())), move_color);

                //追加されるPについてKP,PPを足す
                for (int i = 0; i < PIECE_STATE_LIST_SIZE; i++) {
                    if (i == change_index) {
                        ee_.piece_state_list[i] = add;
                    }
                    kp_score_ += ptr->kp[SquareToNum[move.to()]][ee_.piece_state_list[i]];
                    pp_score_ += ptr->pp[add][ee_.piece_state_list[i]];
                }
                //敵玉とのKP
                kp_score_ -= ptr->kp[ee_.white_king_sq_reversed][invPieceState(add)];

            } else { //後手玉を動かす場合
                PieceState captured = pieceState(move.capture(), move.to(), ~move_color);
                int change_index = -1;
                //captureされたPについてKP, PPを引く
                for (int i = 0; i < PIECE_STATE_LIST_SIZE; i++) {
                    kp_score_ += ptr->kp[SquareToNum[InvSquare[move.from()]]][invPieceState(ee_.piece_state_list[i])];
                    pp_score_ -= ptr->pp[captured][ee_.piece_state_list[i]];
                    if (captured == ee_.piece_state_list[i]) {
                        change_index = i;
                    }
                }
                //敵玉とのKP
                kp_score_ -= ptr->kp[ee_.black_king_sq][captured];

                assert(change_index != -1);

                PieceState add = pieceState(kind(move.capture()), hand_[move_color].num(kind(move.capture())), move_color);

                //追加されるPについてKP,PPを足す
                for (int i = 0; i < PIECE_STATE_LIST_SIZE; i++) {
                    if (i == change_index) {
                        ee_.piece_state_list[i] = add;
                    }
                    kp_score_ -= ptr->kp[SquareToNum[InvSquare[move.to()]]][invPieceState(ee_.piece_state_list[i])];
                    pp_score_ += ptr->pp[add][ee_.piece_state_list[i]];
                }

                //敵玉とのKP
                kp_score_ += ptr->kp[ee_.black_king_sq][add];
            }
        }
    } else if (move.isDrop()) { //駒を打つ手
                                //変化するのはKP, PP両方
                                //打つために消えるKP,PPを引く
        PieceState dropped_from = pieceState(kind(move.subject()), hand_[pieceToColor(move.subject())].num(move.subject()) + 1, pieceToColor(move.subject()));
        int change_index = -1;
        //KP
        kp_score_ -= ptr->kp[ee_.black_king_sq][dropped_from];
        kp_score_ += ptr->kp[ee_.white_king_sq_reversed][invPieceState(dropped_from)];
        //PP
        for (int i = 0; i < PIECE_STATE_LIST_SIZE; i++) {
            pp_score_ -= ptr->pp[dropped_from][ee_.piece_state_list[i]];
            if (dropped_from == ee_.piece_state_list[i]) {
                change_index = i;
            }
        }

        assert(change_index != -1);

        PieceState dropped_to = pieceState(move.subject(), move.to(), pieceToColor(move.subject()));

        //KP
        kp_score_ += ptr->kp[ee_.black_king_sq][dropped_to];
        kp_score_ -= ptr->kp[ee_.white_king_sq_reversed][invPieceState(dropped_to)];
        //PP
        for (int i = 0; i < PIECE_STATE_LIST_SIZE; i++) {
            if (i == change_index) {
                ee_.piece_state_list[i] = dropped_to;
            }
            pp_score_ += ptr->pp[dropped_to][ee_.piece_state_list[i]];
        }
    } else {
        if (move.capture() == EMPTY) {
            //変化するのはKP, PP両方
            //打つために消えるKP,PPを引く
            PieceState removed_from = pieceState(move.subject(), move.from(), pieceToColor(move.subject()));
            int change_index = -1;
            //KP
            kp_score_ -= ptr->kp[ee_.black_king_sq][removed_from];
            kp_score_ += ptr->kp[ee_.white_king_sq_reversed][invPieceState(removed_from)];
            //PP
            for (int i = 0; i < PIECE_STATE_LIST_SIZE; i++) {
                pp_score_ -= ptr->pp[removed_from][ee_.piece_state_list[i]];
                if (removed_from == ee_.piece_state_list[i]) {
                    change_index = i;
                }
            }

            assert(change_index != -1);

            PieceState added_to = pieceState(move.isPromote() ? promote(move.subject()) : move.subject(), move.to(), pieceToColor(move.subject()));

            //KP
            kp_score_ += ptr->kp[ee_.black_king_sq][added_to];
            kp_score_ -= ptr->kp[ee_.white_king_sq_reversed][invPieceState(added_to)];
            //PP
            for (int i = 0; i < PIECE_STATE_LIST_SIZE; i++) {
                if (i == change_index) {
                    ee_.piece_state_list[i] = added_to;
                }
                pp_score_ += ptr->pp[added_to][ee_.piece_state_list[i]];
            }
        } else {
            //2つPが消えて2つPが増える.厄介
            PieceState removed1 = pieceState(move.subject(), move.from(), pieceToColor(move.subject()));
            PieceState removed2 = pieceState(move.capture(), move.to(), pieceToColor(move.capture()));
            int change_index1 = -1, change_index2 = -1;
            //KP
            kp_score_ -= ptr->kp[ee_.black_king_sq][removed1];
            kp_score_ += ptr->kp[ee_.white_king_sq_reversed][invPieceState(removed1)];
            kp_score_ -= ptr->kp[ee_.black_king_sq][removed2];
            kp_score_ += ptr->kp[ee_.white_king_sq_reversed][invPieceState(removed2)];
            //PP
            for (int i = 0; i < PIECE_STATE_LIST_SIZE; i++) {
                pp_score_ -= ptr->pp[removed1][ee_.piece_state_list[i]];
                pp_score_ -= ptr->pp[removed2][ee_.piece_state_list[i]];
                if (removed1 == ee_.piece_state_list[i]) {
                    change_index1 = i;
                }
                if (removed2 == ee_.piece_state_list[i]) {
                    change_index2 = i;
                }
            }
            //同じものを2回引いているので補正する
            pp_score_ += ptr->pp[removed1][removed2];

            assert(change_index1 != -1 && change_index2 != -1);

            PieceState added1 = pieceState(move.isPromote() ? promote(move.subject()) : move.subject(), move.to(), pieceToColor(move.subject()));
            PieceState added2 = pieceState(kind(move.capture()), hand_[pieceToColor(move.subject())].num(move.capture()), pieceToColor(move.subject()));

            //KP
            kp_score_ += ptr->kp[ee_.black_king_sq][added1];
            kp_score_ -= ptr->kp[ee_.white_king_sq_reversed][invPieceState(added1)];
            kp_score_ += ptr->kp[ee_.black_king_sq][added2];
            kp_score_ -= ptr->kp[ee_.white_king_sq_reversed][invPieceState(added2)];

            //PP
            for (int i = 0; i < PIECE_STATE_LIST_SIZE; i++) {
                if (i == change_index1) {
                    ee_.piece_state_list[i] = added1;
                }
                if (i == change_index2) {
                    ee_.piece_state_list[i] = added2;
                }

                pp_score_ += ptr->pp[added1][ee_.piece_state_list[i]];
                pp_score_ += ptr->pp[added2][ee_.piece_state_list[i]];
            }
            //同じものを2回足しているので補正する
            pp_score_ -= ptr->pp[added1][added2];
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

    return piece_score_ + (kp_score_ + pp_score_) / 32;
}