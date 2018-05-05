#include"position.hpp"
#include"piece.hpp"
#include"move.hpp"
#include"common.hpp"
#include"piece_state.hpp"
#include<iostream>
#include<cstdio>
#include<ctime>
#include<bitset>
#include<cassert>
#include<iterator>
#include<algorithm>
#include<set>

Position::Position() {
	//盤上の初期化
	for (int i = 0; i < SquareNum; i++) board_[i] = WALL;
	for (Square sq : SquareList) board_[sq] = EMPTY;

	//後手の駒
	board_[SQ11] = WHITE_LANCE;
	board_[SQ21] = WHITE_KNIGHT;
	board_[SQ31] = WHITE_SILVER;
	board_[SQ41] = WHITE_GOLD;
	board_[SQ51] = WHITE_KING;
	board_[SQ61] = WHITE_GOLD;
	board_[SQ71] = WHITE_SILVER;
	board_[SQ81] = WHITE_KNIGHT;
	board_[SQ91] = WHITE_LANCE;
	board_[SQ22] = WHITE_BISHOP;
	board_[SQ82] = WHITE_ROOK;
	board_[SQ13] = WHITE_PAWN;
	board_[SQ23] = WHITE_PAWN;
	board_[SQ33] = WHITE_PAWN;
	board_[SQ43] = WHITE_PAWN;
	board_[SQ53] = WHITE_PAWN;
	board_[SQ63] = WHITE_PAWN;
	board_[SQ73] = WHITE_PAWN;
	board_[SQ83] = WHITE_PAWN;
	board_[SQ93] = WHITE_PAWN;

	//先手の駒
	board_[SQ19] = BLACK_LANCE;
	board_[SQ29] = BLACK_KNIGHT;
	board_[SQ39] = BLACK_SILVER;
	board_[SQ49] = BLACK_GOLD;
	board_[SQ59] = BLACK_KING;
	board_[SQ69] = BLACK_GOLD;
	board_[SQ79] = BLACK_SILVER;
	board_[SQ89] = BLACK_KNIGHT;
	board_[SQ99] = BLACK_LANCE;
	board_[SQ88] = BLACK_BISHOP;
	board_[SQ28] = BLACK_ROOK;
	board_[SQ17] = BLACK_PAWN;
	board_[SQ27] = BLACK_PAWN;
	board_[SQ37] = BLACK_PAWN;
	board_[SQ47] = BLACK_PAWN;
	board_[SQ57] = BLACK_PAWN;
	board_[SQ67] = BLACK_PAWN;
	board_[SQ77] = BLACK_PAWN;
	board_[SQ87] = BLACK_PAWN;
	board_[SQ97] = BLACK_PAWN;

	//持ち駒
	hand_[BLACK].clear();
	hand_[WHITE].clear();

	//手番
	color_ = BLACK;

	//手数
	turn_number_ = 0;

	//玉の位置
	king_sq_[BLACK] = SQ59;
	king_sq_[WHITE] = SQ51;

	//局面の評価値
	initScore();

	//ハッシュ値
	hashInit();
    hash_value_ = 0;
    board_hash_ = 0;
    hand_hash_ = 0;
    for (auto sq : SquareList) {
        board_hash_ ^= HashSeed[board_[sq]][sq];
    }
	for (Piece piece = PAWN; piece <= ROOK; piece++) {
		hand_hash_ ^= HandHashSeed[BLACK][piece][hand_[BLACK].num(piece)];
		hand_hash_ ^= HandHashSeed[WHITE][piece][hand_[WHITE].num(piece)];
	}
    hash_value_ = board_hash_ ^ hand_hash_;
    hash_value_ &= ~1; //これで1bit目が0になる(先手番を表す)

    ee_ = makeEvalElements();

    stack_.reserve(512);
    kifu_.reserve(512);

    //Bitboard
    occupied_bb_[BLACK] = Bitboard(0, 0);
    occupied_bb_[WHITE] = Bitboard(0, 0);
    for (int p = 0; p < PieceNum; ++p) {
        pieces_bb_[p] = Bitboard(0, 0);
    }
    for (Square sq : SquareList) {
        if (board_[sq] != EMPTY) {
            pieces_bb_[board_[sq]] |= SQUARE_BB[sq];
            occupied_bb_[pieceToColor(board_[sq])] |= SQUARE_BB[sq];
        }
    }
    occupied_all_ = occupied_bb_[BLACK] | occupied_bb_[WHITE];

    //pinners
    computePinners();


    //利きを設定
    initControl();

    isChecked_ = false;
}

void Position::print() const {
	//盤上
	std::printf("９８７６５４３２１\n");
	std::printf("------------------\n");
	for (int r = Rank1; r <= Rank9; r++) {
		for (int f = File9; f >= File1; f--) {
			std::cout << PieceToSfenStr[board_[FRToSquare[f][r]]];
		}
		printf("|%d\n", r);
	}

	//先手の持ち駒
	printf("持ち駒\n");
	printf("先手:");
	hand_[BLACK].print();

	//後手の持ち駒
	printf("後手:");
	hand_[WHITE].print();

	//手番
	printf("手番:");
	if (color_ == BLACK) printf("先手\n");
	else printf("後手\n");

	//手数
	printf("手数:%d\n", turn_number_);

	//最後の手
	if (!kifu_.empty()) {
		printf("最後の手:");
		lastMove().print();
	}

	//評価値
	printf("駒割評価値:%d\n", piece_score_);
	printf("合計評価値:%d\n", score());

	printf("ハッシュ値:%lld\n", hash_value_);

    printControl();
}

void Position::printAllMoves() const {
	std::vector<Move> moves = generateAllMoves();
	printf("全合法手の数:%zu\n", moves.size());
	for (Move move : moves) move.print();
}

void Position::printHistory() const {
	printf("print history\n");
	for (Move move : kifu_) move.print();
	printf("\n");
}

void Position::printForDebug() const {
    print();
    printHistory();
    //printAllMoves();
    //std::cout << control_bb_(BLACK) << std::endl;
    //std::cout << control_bb_(WHITE) << std::endl;
}

void Position::doMove(const Move move) {
    extern int piece_value[];

#if DEBUG
    if (!isLegalMove(move)) {
        printForDebug();
        std::cout << "違法だった手:";
        move.print();
        isLegalMove(move);
        assert(false);
    }
#endif

    //動かす前の状態を残しておく
    stack_.emplace_back(*this);

	//実際に動かす
	if (move.isDrop()) { //持ち駒を打つ手

		//持ち駒を減らす
		hand_[color_].sub(kind(move.subject()));

		//移動先にsubjectを設置
		board_[move.to()] = move.subject();

		//ハッシュ値の更新
		//打つ前のHandHashとXORして消す
		hand_hash_ ^= HandHashSeed[color_][kind(move.subject())][hand_[color_].num(kind(move.subject())) + 1];
		//打った後のHandHashとXOR
		hand_hash_ ^= HandHashSeed[color_][kind(move.subject())][hand_[color_].num(kind(move.subject()))];
		//打った後の分をXOR
		board_hash_ ^= HashSeed[move.subject()][move.to()];

        //評価値の更新
        piece_score_ -= (color_ == BLACK ? piece_value[kind(move.subject())] : -piece_value[kind(move.subject())]);
        piece_score_ += piece_value[move.subject()];

        //Bitboard更新
        pieces_bb_[move.subject()] |= SQUARE_BB[move.to()];
        occupied_bb_[color_] |= SQUARE_BB[move.to()];

        //利きの更新
        putPiece(move.subject(), move.to());

    } else { //盤上の駒を動かす手
		
		//移動する駒を消す
		board_[move.from()] = EMPTY;
        pieces_bb_[move.subject()] &= ~SQUARE_BB[move.from()];
        occupied_bb_[color_] &= ~SQUARE_BB[move.from()];

        //利きを消す
        removePiece(move.subject(), move.from());

		//取った駒があるならその駒を消し、持ち駒を増やす
		if (move.capture() != EMPTY) {
            //取った駒を消す
			board_[move.to()] = EMPTY;
            pieces_bb_[move.capture()] &= ~SQUARE_BB[move.to()];
            occupied_bb_[~color_] &= ~SQUARE_BB[move.to()];

            //利きを消す
            removePiece(move.capture(), move.to());
            
            //持ち駒を増やす
            hand_[color_].add(kind(move.capture()));

			//ハッシュ値の更新
			//取った駒分のハッシュをXOR
			board_hash_ ^= HashSeed[move.capture()][move.to()];
			//増える前の持ち駒の分をXORして消す
			hand_hash_ ^= HandHashSeed[color_][kind(move.capture())][hand_[color_].num(kind(move.capture())) - 1];
			//増えた後の持ち駒の分XOR
			hand_hash_ ^= HandHashSeed[color_][kind(move.capture())][hand_[color_].num(kind(move.capture()))];

            //評価値の更新
            piece_score_ += (color_ == BLACK ? piece_value[kind(move.capture())] : -piece_value[kind(move.capture())]);
            piece_score_ -= piece_value[move.capture()];
		}

		//成る手ならsubjectに成りのフラグを立てて,そうでないならsubjectをそのまま移動先に設置
        if (move.isPromote()) {
            board_[move.to()] = promote(move.subject());

            //利きの更新
            putPiece(promote(move.subject()), move.to());

            //評価値の更新
            piece_score_ += piece_value[promote(move.subject())];
            piece_score_ -= piece_value[move.subject()];

            //Bitboard更新
            pieces_bb_[promote(move.subject())] |= SQUARE_BB[move.to()];
        } else {
            board_[move.to()] = move.subject();

            //利きの更新
            putPiece(move.subject(), move.to());

            //Bitboard更新
            pieces_bb_[move.subject()] |= SQUARE_BB[move.to()];
        }
        //occupiedは成か不成かによらない
        occupied_bb_[color_] |= SQUARE_BB[move.to()];

		//ハッシュ値の更新
		//移動前の分をXORして消す
		board_hash_ ^= HashSeed[move.subject()][move.from()];
		//移動後の分をXOR
        if (move.isPromote())
            board_hash_ ^= HashSeed[promote(move.subject())][move.to()];
        else
            board_hash_ ^= HashSeed[move.subject()][move.to()];
	}

	//玉を動かす手ならblack_king_pos,white_king_posに反映
    if (kind(move.subject()) == KING) {
        king_sq_[color_] = move.to();
        if (color_ == BLACK) {
            ee_.black_king_sq = SquareToNum[move.to()];
        } else {
            ee_.white_king_sq_reversed = SquareToNum[InvSquare[move.to()]];
        }
    }

    //occupied_all_を更新
    occupied_all_ = occupied_bb_[BLACK] | occupied_bb_[WHITE];

	//手番の更新
	color_ = ~color_;

    //Bitboard関連
    //王手
    isChecked_ = isThereControl(~color_, king_sq_[color_]);

    //pinners
    computePinners();

	//手数の更新
	turn_number_++;

	//棋譜に指し手を追加
	kifu_.push_back(move);

	//hashの手番要素を更新
    hash_value_ = board_hash_ ^ hand_hash_;
	//1bit目を0にする
	hash_value_ &= ~1;
	//手番が先手だったら1bitは0のまま,後手だったら1bit目は1になる
	hash_value_ |= color_;

    calcScoreDiff();
}

void Position::undo() {
    //駒割のテーブル読み込み
    extern int piece_value[];

	const Move last_move = kifu_.back();
	kifu_.pop_back();

	//手番を戻す(このタイミングでいいかな?)
	color_ = ~color_;

	//動かした駒を消す
	board_[last_move.to()] = EMPTY;
    occupied_bb_[color_] &= ~SQUARE_BB[last_move.to()];

	//盤の状態を戻す
	if (last_move.isDrop()) { //打つ手

		//持ち駒を増やす
		hand_[color_].add(kind(last_move.subject()));

		//ハッシュ値の巻き戻し
		//戻す前のHandHashとXOR
		hand_hash_ ^= HandHashSeed[color_][kind(last_move.subject())][hand_[color_].num(kind(last_move.subject())) - 1];
		//戻す前の分をXORして消す
		board_hash_ ^= HashSeed[last_move.subject()][last_move.to()];
		//戻した後のHandHashとXOR
		hand_hash_ ^= HandHashSeed[color_][kind(last_move.subject())][hand_[color_].num(kind(last_move.subject()))];

        //評価値の更新
        piece_score_ += (color_ == BLACK ? piece_value[kind(last_move.subject())] : -piece_value[kind(last_move.subject())]);
        piece_score_ -= piece_value[last_move.subject()];

        //Bitboard更新
        pieces_bb_[last_move.subject()] &= ~SQUARE_BB[last_move.to()];

        //利きの更新
        removePiece(last_move.subject(), last_move.to());
	} else { //盤上の駒を動かす手
        //動いた駒の利きを削除する
        removePiece(last_move.isPromote() ? promote(last_move.subject()) : last_move.subject(), last_move.to());

		//取る手だったらtoに取った駒を戻し、持ち駒を減らす
		if (last_move.capture() != EMPTY) {
			board_[last_move.to()] = last_move.capture();
			hand_[color_].sub(kind(last_move.capture()));

            //利きの更新
            putPiece(last_move.capture(), last_move.to());

            //Bitboard更新
            pieces_bb_[last_move.capture()] |= SQUARE_BB[last_move.to()];
            occupied_bb_[~color_] |= SQUARE_BB[last_move.to()];

			//ハッシュ値の巻き戻し
			//取る前の分のハッシュをXOR
			board_hash_ ^= HashSeed[last_move.capture()][last_move.to()];
			//増える前の持ち駒の分
			hand_hash_ ^= HandHashSeed[color_][last_move.capture() & PIECE_KIND_MASK][hand_[color_].num(kind(last_move.capture()))];
			//増えた後の持ち駒の分XORして消す
			hand_hash_ ^= HandHashSeed[color_][last_move.capture() & PIECE_KIND_MASK][hand_[color_].num(kind(last_move.capture())) + 1];

            //評価値の更新
            piece_score_ -= (color_ == BLACK ? piece_value[kind(last_move.capture())] : -piece_value[kind(last_move.capture())]);
            piece_score_ += piece_value[last_move.capture()];
		}

		//動いた駒をfromに戻す
		board_[last_move.from()] = last_move.subject();
        //利きを戻る
        putPiece(last_move.subject(), last_move.from());
        //Bitboard更新
        pieces_bb_[last_move.subject()] |= SQUARE_BB[last_move.from()];
        occupied_bb_[color_] |= SQUARE_BB[last_move.from()];

		//ハッシュ値の巻き戻し
		//移動前の分をXOR
    	board_hash_ ^= HashSeed[last_move.subject()][last_move.from()];
		//移動後の分をXORして消す
        if (last_move.isPromote())
    		board_hash_ ^= HashSeed[promote(last_move.subject())][last_move.to()];
        else
            board_hash_ ^= HashSeed[last_move.subject()][last_move.to()];

        //評価値とBitboardの更新
        if (last_move.isPromote()) {
            //評価値の更新
            piece_score_ -= piece_value[promote(last_move.subject())];
            piece_score_ += piece_value[last_move.subject()];

            pieces_bb_[promote(last_move.subject())] &= ~SQUARE_BB[last_move.to()];
        } else {
            pieces_bb_[last_move.subject()] &= ~SQUARE_BB[last_move.to()];
        }
	}

	//玉を動かす手ならking_sq_に反映
	if (kind(last_move.subject()) == KING) king_sq_[color_] = last_move.from();
    
    //occupied_all_を更新
    occupied_all_ = occupied_bb_[BLACK] | occupied_bb_[WHITE];

    //王手は自玉へのattackers
    //checkers_ = attackersTo(~color_, king_sq_[color_]);

	//ハッシュの更新
    hash_value_ = board_hash_ ^ hand_hash_;
	//一番右のbitを0にする
	hash_value_ &= ~1;
	//一番右のbitが先手番だったら0のまま、後手番だったら1になる
	hash_value_ |= color_;

	//手数
	turn_number_--;

    //計算が面倒なものはコピーで戻してみる
    piece_score_ = stack_.back().piece_score;
    kp_score_ = stack_.back().kp_score;
    pp_score_ = stack_.back().pp_score;
    ee_ = stack_.back().features;
    pinners_ = stack_.back().pinners;
    isChecked_ = stack_.back().isChecked;

    //Stack更新
    stack_.pop_back();
}

void Position::doNullMove() {
    stack_.emplace_back(*this);

    //手番の更新
    color_ = ~color_;

    //手数の更新
    turn_number_++;

    //hashの手番要素を更新
    //1bit目を0にする
    hash_value_ &= ~1;
    //手番が先手だったら1bitは0のまま,後手だったら1bit目は1になる
    hash_value_ |= color_;

    kifu_.push_back(NULL_MOVE);

    //pinnersとpinned
    computePinners();
}

void Position::undoNullMove() {
    kifu_.pop_back();

    //手番を戻す(このタイミングでいいかな?)
    color_ = ~color_;

    //ハッシュの更新(手番分)
    //一番右のbitを0にする
    hash_value_ &= ~1;
    //一番右のbitが先手番だったら0のまま、後手番だったら1になる
    hash_value_ |= color_;

    //手数
    turn_number_--;

    //計算が面倒なものはコピーで戻してみる
    piece_score_ = stack_.back().piece_score;
    kp_score_ = stack_.back().kp_score;
    pp_score_ = stack_.back().pp_score;
    ee_ = stack_.back().features;
    pinners_ = stack_.back().pinners;
    isChecked_ = stack_.back().isChecked;

    //Stack更新
    stack_.pop_back();
}

bool Position::isLegalMove(const Move move) const {
	//違法の場合だけ早くfalseで返す.合法手は一番最後の行でtrueが返る

	//NULL_MOVEだけは先に判定するか……
	if (move == NULL_MOVE) {
		return false;
	}

	//打つ手と動かす手両方に共通するもの
	//移動先のチェック
	if (!isOnBoard(move.to())) {
#if DEBUG
		std::cout << "移動先が盤上ではありません" << std::endl;
#endif
		return false;
	}
	if (color_ == BLACK && pieceToColor(board_[move.to()]) == BLACK) {
#if DEBUG
		std::cout << "先手の移動先に先手の駒があります" << std::endl;
#endif
		return false;
	}
	if (color_ == WHITE && pieceToColor(board_[move.to()]) == WHITE) {
#if DEBUG
		std::cout << "後手の移動先に後手の駒があります" << std::endl;
#endif
		return false;
	}

    if (!move.isDrop() && move.subject() != board_[move.from()]) {
#if DEBUG
        std::cout << "動かす駒が違います" << std::endl;
#endif
        return false;
    }

    if (move.subject() == EMPTY) {
#if DEBUG
        std::cout << "無を動かしています" << std::endl;
#endif
        return false;
    }

	//手番と動かす駒の対応チェック
	if (color_ == BLACK && (pieceToColor(move.subject()) == WHITE)) {
#if DEBUG
		std::cout << "先手の指し手で後手の駒を動かしています" << std::endl;
#endif
		return false;
	}
	if (color_ == WHITE && (pieceToColor(move.subject()) == BLACK)) {
#if DEBUG
		std::cout << "後手の指し手で先手の駒を動かしています" << std::endl;
#endif
		return false;
	}

    //取る駒の対応
    if (move.capture() != board_[move.to()]) {
#if DEBUG
        std::cout << "captureとboard_[move.to()]が食い違っています" << std::endl;
#endif
        return false;
    }

	//成りのチェック
	if (move.isPromote() && color_ == BLACK) {
		if (SquareToRank[move.to()] > Rank3 && SquareToRank[move.from()] > Rank3) {
#if DEBUG
			std::cout << "自陣で成っています" << std::endl;
#endif
			return false;
		}
	}
	if (move.isPromote() && color_ == WHITE) {
		if (SquareToRank[move.to()] < Rank7 && SquareToRank[move.from()] < Rank7) {
#if DEBUG
			std::cout << "自陣で成っています" << std::endl;
#endif
			return false;
		}
	}
	if (move.isDrop()) { //駒を打つ手特有の判定
		//打つ先が空になってるか
		if (board_[move.to()] != EMPTY) {
#if DEBUG
			std::cout << "打つ先に駒があります" << std::endl;
#endif
			return false;
		}
        //打つ駒はあるか
        if (hand_[color_].num(kind(move.subject())) <= 0) {
#if DEBUG
            std::cout << "打つ駒がありません" << std::endl;
#endif
            return false;
        }

		//二歩になっていないか
		if (move.subject() == BLACK_PAWN && !canDropPawn(move.to())) {
#if DEBUG
			std::cout << "二歩または打ち歩詰めです" << std::endl;
#endif
			return false;
		}
		if (move.subject() == WHITE_PAWN && !canDropPawn(move.to())) {
#if DEBUG
			std::cout << "二歩または打ち歩詰めです" << std::endl;
#endif
			return false;
		}
		//歩を最奥段に打っていないか
		if (move.subject() == BLACK_PAWN && SquareToRank[move.to()] == Rank1) {
#if DEBUG
			std::cout << "一段目に歩を打っています" << std::endl;
#endif
			return false;
		}
		if (move.subject() == WHITE_PAWN && SquareToRank[move.to()] == Rank9) {
#if DEBUG
			std::cout << "九段目に歩を打っています" << std::endl;
#endif
			return false;
		}
		//香を最奥段に打っていないか
		if (move.subject() == BLACK_LANCE && SquareToRank[move.to()] == Rank1) {
#if DEBUG
			std::cout << "一段目に香を打っています" << std::endl;
#endif
			return false;
		}
		if (move.subject() == WHITE_LANCE && SquareToRank[move.to()] == Rank9) {
#if DEBUG
			std::cout << "九段目に香を打っています" << std::endl;
#endif
			return false;
		}
		//桂を最奥段に打っていないか
		if (move.subject() == BLACK_KNIGHT && SquareToRank[move.to()] == Rank1) {
#if DEBUG
			std::cout << "一段目に桂を打っています" << std::endl;
#endif
			return false;
		}
		if (move.subject() == WHITE_KNIGHT && SquareToRank[move.to()] == Rank9) {
#if DEBUG
			std::cout << "九段目に桂を打っています" << std::endl;
#endif
			return false;
		}
		//桂を奥から二段目に打っていないか
		if (move.subject() == BLACK_KNIGHT && SquareToRank[move.to()] == Rank2) {
#if DEBUG
			std::cout << "二段目に香を打っています" << std::endl;
#endif
			return false;
		}
		if (move.subject() == WHITE_KNIGHT && SquareToRank[move.to()] == Rank8) {
#if DEBUG
            std::cout << "八段目に香を打っています" << std::endl;
#endif
			return false;
		}
	} else { //盤上の駒を動かす手の判定
		//各駒に合わせた動きになっているか
		bool flag = false;
        for (auto delta : CanMove[board_[move.from()]]) {
            if (move.to() == move.from() + delta)
                flag = true;
        }

        if (BETWEEN_BB[move.from()][move.to()] & occupied_all_) {
#if DEBUG
            std::cout << "駒を飛び越えています" << std::endl;
#endif
            return false;
        }
	}

	//自殺手
	if (kind(move.subject()) == KING && isThereControl(~color_, move.to())) {
#if DEBUG
		std::cout << "自殺手" << std::endl;
#endif
		return false;
	}

    bool flag = true;
    pinners_.forEach([&](const Square pinner_sq) {
        if (BETWEEN_BB[pinner_sq][king_sq_[color_]] & SQUARE_BB[move.from()]) { //fromがbetween,すなわちピンされている
            if ((BETWEEN_BB[pinner_sq][king_sq_[color_]] | SQUARE_BB[pinner_sq]) & SQUARE_BB[move.to()]) {
                //toがbetween内及びpinner_sqだったらOK
            } else {
                flag = false;
            }
        }
    });
    if (!flag) {
#if DEBUG
        std::cout << "ピンされた駒が違法な動き" << std::endl;
#endif
        return false;
    }

	//玉を取る手がなぜか発生する場合
	if (kind(board_[move.to()]) == KING) {
#if DEBUG
		std::cout << "玉を取る手" << std::endl;
#endif
		return false;
	}
	return true;
}

bool Position::canDropPawn(const Square to) const {
    //2歩の判定を入れる
    if (FILE_BB[SquareToFile[to]] & pieces_bb_[color_ == BLACK ? toBlack(PAWN) : toWhite(PAWN)]) {
        return false;
    }

    //打ち歩詰めの判定
    //冷静に考えなきゃ
    //打ち歩詰めの条件は
    //(1)玉の前に打っている
    //(2)それを取れない
    //(2)-a:玉で取れない(打った歩に紐が付いている)
    //(2)-b:他の駒で取れない(利きがない)
    //(3)逃げ場もない
    //かな
    if (controlBB(king_sq_[~color_], (color_ == BLACK ? toWhite(PAWN) : toBlack(PAWN)), occupied_all_) & SQUARE_BB[to] //(1)
        && isThereControl(color_, to) //(2)-a
        && controls_[~color_][to].number <= 1 //(2)-b 利きが1(相手玉自身の利きだけ)
        ) { //これで(1),(2)が満たされたので逃げ場を探す
        bool can_evasion = false;
        Bitboard evasion_to_bb = controlBB(king_sq_[~color_], board_[king_sq_[~color_]], occupied_all_) & ~occupied_bb_[~color_];
        evasion_to_bb.forEach([&](const Square evasion_to) {
            if (!attackersTo(color_, evasion_to, occupied_all_ | SQUARE_BB[to])) {
                can_evasion = true;
            }
        });
        if (!can_evasion) {
            return false;
        }
    }
	return true;
}

bool Position::isMoreControl(const Square to) const {
    return controls_[color_][to].number > controls_[~color_][to].number;
    //return attackersTo(color_, to).pop_count() > attackersTo(~color_, to).pop_count();
}

void Position::load_sfen(std::string sfen) {
	//初期化
	for (int i = 0; i < SquareNum; i++) board_[i] = WALL;
	for (Square sq : SquareList) board_[sq] = EMPTY;

	//テーブル用意しておいたほうがシュッと書ける
	static std::unordered_map<char, Piece> CharToPiece = {
		{ 'P', BLACK_PAWN },
		{ 'L', BLACK_LANCE },
		{ 'N', BLACK_KNIGHT },
		{ 'S', BLACK_SILVER },
		{ 'G', BLACK_GOLD },
		{ 'B', BLACK_BISHOP },
		{ 'R', BLACK_ROOK },
		{ 'K', BLACK_KING },
		{ 'p', WHITE_PAWN },
		{ 'l', WHITE_LANCE },
		{ 'n', WHITE_KNIGHT },
		{ 's', WHITE_SILVER },
		{ 'g', WHITE_GOLD },
		{ 'b', WHITE_BISHOP },
		{ 'r', WHITE_ROOK },
		{ 'k', WHITE_KING },
	};

	//sfen文字列を走査するイテレータ(ダサいやり方な気がするけどパッと思いつくのはこれくらい)
	unsigned int i;

	//盤上の設定
	int r = Rank1, f = File9;
	for (i = 0; i < sfen.size(); i++) {
		if (sfen[i] == '/') {
			//次の段へ移る
			f = File9;
			r++;
		} else if (sfen[i] == ' ') {
			//手番の設定へ
			break;
		} else if ('1' <= sfen[i] && sfen[i] <= '9') {
			//空マス分飛ばす
			f -= sfen[i] - '0';
		} else if (sfen[i] == '+') {
			//次の文字が示す駒を成らせてboard_に設置
			board_[FRToSquare[f--][r]] = promote(CharToPiece[sfen[++i]]);
		} else {
			//玉だったらking_sq_を設定
			if (CharToPiece[sfen[i]] == BLACK_KING) king_sq_[BLACK] = FRToSquare[f][r];
			else if (CharToPiece[sfen[i]] == WHITE_KING) king_sq_[WHITE] = FRToSquare[f][r];
			//文字が示す駒をboard_に設置
			board_[FRToSquare[f--][r]] = CharToPiece[sfen[i]];
		}
	}

	//手番の設定
	if (sfen[++i] == 'b') color_ = BLACK;
	else color_ = WHITE;
	
	//美しくない操作だ……
	i += 2;

	//持ち駒
	hand_[BLACK].clear();
	hand_[WHITE].clear();
    int num = 1;
	while (sfen[i] != ' ') {
        if ('1' <= sfen[i] && sfen[i] <= '9') { //数字なら枚数の取得
            if ('0' <= sfen[i + 1] && sfen[i + 1] <= '9') {
                //次の文字も数字の場合が一応あるはず(歩が10枚以上)
                num = 10 * (sfen[i] - '0') + sfen[i + 1] - '0';
                i += 2;
            } else {
                //次が数字じゃないなら普通に取る
                num = sfen[i++] - '0';
            }
        } else { //駒なら持ち駒を変更
            Piece piece = CharToPiece[sfen[i++]];
            hand_[pieceToColor(piece)].set(kind(piece), num);

            //枚数を1に戻す
            num = 1;
        }
	}

	//手数
	turn_number_ = 0;

	//局面の評価値
	initScore();

    //ハッシュ値
    hashInit();
    hash_value_ = 0;
    board_hash_ = 0;
    hand_hash_ = 0;
    for (auto sq : SquareList) {
        board_hash_ ^= HashSeed[board_[sq]][sq];
    }
    for (Piece piece = PAWN; piece <= ROOK; piece++) {
        hand_hash_ ^= HandHashSeed[BLACK][piece][hand_[BLACK].num(piece)];
        hand_hash_ ^= HandHashSeed[WHITE][piece][hand_[WHITE].num(piece)];
    }
    hash_value_ = board_hash_ ^ hand_hash_;
    hash_value_ &= ~1; //これで1bit目が0になる(先手番を表す)

    ee_ = makeEvalElements();

    stack_.reserve(256);

    //Bitboard
    occupied_bb_[BLACK] = Bitboard(0, 0);
    occupied_bb_[WHITE] = Bitboard(0, 0);
    for (int p = 0; p < PieceNum; ++p) {
        pieces_bb_[p] = Bitboard(0, 0);
    }
    for (Square sq : SquareList) {
        if (board_[sq] != EMPTY) {
            pieces_bb_[board_[sq]] |= SQUARE_BB[sq];
            occupied_bb_[pieceToColor(board_[sq])] |= SQUARE_BB[sq];
        }
    }
    occupied_all_ = occupied_bb_[BLACK] | occupied_bb_[WHITE];

    //pinners
    computePinners();
}

void Position::hashInit() {
	std::mt19937_64 rnd((int)time(0));
	for (int piece = BLACK_PAWN; piece <= WHITE_ROOK_PROMOTE; piece++) {
		for (Square sq : SquareList) {
			HashSeed[piece][sq] = rnd();
		}
	}
	for (int color = BLACK; color <= WHITE; color++) {
		for (int piece = PAWN; piece <= ROOK; piece++) {
			for (int num = 0; num <= 18; num++) {
				HandHashSeed[color][piece][num] = rnd();
			}
		}
	}
}

void Position::generateEvasionMovesBB(Move *& move_ptr) const {
    //王手への対応は(a)玉が逃げる、(b)王手してきた駒を取る、(c)飛び利きの王手には合駒

    //手番のほうの玉の位置を設定
    Square evasion_from = king_sq_[color_];

    //(a)逃げる手
    //隣接王手を玉で取り返せる場合はここに含む
    Bitboard king_to_bb = controlBB(evasion_from, board_[evasion_from], occupied_all_);

    //味方の駒がいる位置にはいけないので除く
    king_to_bb &= ~occupied_bb_[color_];

    if (controls_[~color_][evasion_from].direction != 0) {
        //長い利きがある反対方向には逃げられない
        for (auto long_control : LongControlList) {
            if (long_control & controls_[~color_][evasion_from].direction) {
                Dir d = ConDirToOppositeDir[long_control];
                king_to_bb &= ~SQUARE_BB[evasion_from + d];
            }
        }
    }

    king_to_bb.forEach([&](const Square to) {
        //Move evasion_move(to, evasion_from, false, false, board_[evasion_from], board_[to]);
        ////長い利きが残ってる方に逃げてしまうのでoccupiedから玉のfromを抜く
        //if (!attackersTo(~color_, to, occupied_all_ & ~SQUARE_BB[evasion_from]))
        //    *(move_ptr++) = evasion_move;
        if (controls_[~color_][to].number == 0) { //相手の利きがないときだけそこに動ける
            *(move_ptr++) = Move(to, evasion_from, false, false, board_[evasion_from], board_[to]);
        }
    });

    Bitboard checkers = attackersTo(~color_, king_sq_[color_]);

    //(b),(c)は両王手だったら無理
    if (checkers.pop_count() != 1)
        return;

    //王手してきた駒の位置
    //関数をconst化しているのでchecker_自体は変更できないからコンストラクタで包まないといけない
    const Square checker_sq = checkers.pop();

    //(b)王手してきた駒を玉以外で取る手
    //ピンされた駒を先に動かす
    Bitboard pinned_piece(0, 0);

    pinners_.forEach([&](const Square pinner_sq) {
        //pinnerと自玉の間にあるのがpinされている駒
        Bitboard pinned = BETWEEN_BB[pinner_sq][king_sq_[color_]] & occupied_bb_[color_];

        pinned.forEach([&](const Square from) {
            //取る手なのでpinnerを取る手、かつそこがchecker_sqでないとしかダメ
            Bitboard to_bb = controlBB(from, board_[from], occupied_all_) & SQUARE_BB[pinner_sq] & SQUARE_BB[checker_sq];

            if (to_bb) {
                pushMove(Move(pinner_sq, from, false, false, board_[from], board_[pinner_sq]), move_ptr);
            }

            //使ったマスを記録しておく
            pinned_piece |= SQUARE_BB[from];
        });
    });

    //王手してきた駒を取れる駒の候補
    Bitboard romovers = attackersTo(color_, checker_sq) & ~pinned_piece & ~SQUARE_BB[king_sq_[color_]];

    romovers.forEach([&](Square from) {
        pushMove(Move(checker_sq, from, false, false, board_[from], board_[checker_sq]), move_ptr);
    });

    //(c)合駒
    //王手してきた駒と自玉の間を示すBitboard
    Bitboard between = BETWEEN_BB[checker_sq][king_sq_[color_]];

    //(c)-1 移動合
    Bitboard from_bb = occupied_bb_[color_] & ~pinned_piece & ~SQUARE_BB[king_sq_[color_]];
    from_bb.forEach([&](const Square from) {
        Bitboard to_bb = controlBB(from, board_[from], occupied_all_) & between;
        to_bb.forEach([&](const Square to) {
            pushMove(Move(to, from, false, false, board_[from], board_[to]), move_ptr);
        });
    });

    //(c)-2 打つ合
    //王手されているのでbetweenの示すマスは駒がないはず
    generateDropMoveBB(between, move_ptr);
}

void Position::generateCaptureMovesBB(Move *& move_ptr) const {
    //ピンされた駒を先に動かす
    Bitboard pinned_piece(0, 0);

    pinners_.forEach([&](const Square pinner_sq) {
        //pinnerと自玉の間にあるのがpinされている駒
        Bitboard pinned = BETWEEN_BB[pinner_sq][king_sq_[color_]] & occupied_bb_[color_];
        
        pinned.forEach([&](const Square from) {
            //取る手なのでpinnerを取る手しかダメ
            Bitboard to_bb = controlBB(from, board_[from], occupied_all_) & SQUARE_BB[pinner_sq];

            if (to_bb) {
                pushMove(Move(pinner_sq, from, false, false, board_[from], board_[pinner_sq]), move_ptr);
            }

            //使ったマスを記録しておく
            pinned_piece |= SQUARE_BB[from];
        });
    });

    //玉は別に処理する
    Bitboard king_to_bb = controlBB(king_sq_[color_], board_[king_sq_[color_]], occupied_all_)
        & occupied_bb_[~color_];
    king_to_bb.forEach([&](const Square to) {
        if (!isThereControl(~color_, to)) {
            *(move_ptr++) = Move(to, king_sq_[color_], false, false, board_[king_sq_[color_]], board_[to]);
        }
    });

    //自分の駒がいる位置から、ピンされた駒と玉は先に処理したので除く
    Bitboard from_bb = occupied_bb_[color_] & ~pinned_piece & ~SQUARE_BB[king_sq_[color_]];
    
    from_bb.forEach([&](const Square from) {
        //駒を取る手なので利きの先に相手の駒がないといけない
        Bitboard to_bb = controlBB(from, board_[from], occupied_all_) & occupied_bb_[~color_];

        to_bb.forEach([&](const Square to) {
            pushMove(Move(to, from, false, false, board_[from], board_[to]), move_ptr);
        });
    });
}

void Position::generateNonCaptureMovesBB(Move *& move_ptr) const {
    //ピンされた駒を先に動かす
    Bitboard pinned_piece(0, 0);

    pinners_.forEach([&](const Square pinner_sq) {
        //pinnerと自玉の間にあるのがpinされている駒
        Bitboard pinned = BETWEEN_BB[pinner_sq][king_sq_[color_]] & occupied_bb_[color_];

        pinned.forEach([&](const Square from) {
            //取らない手なのでbetween上を動く手しかダメ(ピンされているのでbetween上に他の駒はない)
            Bitboard to_bb = controlBB(from, board_[from], occupied_all_) & BETWEEN_BB[pinner_sq][king_sq_[color_]];

            to_bb.forEach([&](const Square to) {
                pushMove(Move(to, from, false, false, board_[from], board_[to]), move_ptr);
            });

            //使ったマスを記録しておく
            pinned_piece |= SQUARE_BB[from];
        });
    });

    //王の処理
    Bitboard king_to_bb = controlBB(king_sq_[color_], board_[king_sq_[color_]], occupied_all_)
        & ~occupied_all_;
    king_to_bb.forEach([&](const Square to) {
        //相手の利きがなければそこへいく動きを生成できる
        if (!isThereControl(~color_, to)) {
            *(move_ptr++) = Move(to, king_sq_[color_], false, false, board_[king_sq_[color_]], board_[to]);
        }
    });

    //ピンされた駒と玉は先に処理したので除く
    Bitboard from_bb = occupied_bb_[color_] & ~pinned_piece & ~SQUARE_BB[king_sq_[color_]];
    from_bb.forEach([&](const Square from) {
        //sqにある駒の利き
        Bitboard to_bb = controlBB(from, board_[from], occupied_all_) & ~occupied_all_;
        to_bb.forEach([&](const Square to) {
            pushMove(Move(to, from, false, false, board_[from], board_[to]), move_ptr);
        });
    });

    //駒を打つ手
    Bitboard drop_to_bb = (~occupied_all_ & BOARD_BB);
    generateDropMoveBB(drop_to_bb, move_ptr);
}

void Position::generateRecaptureMovesToBB(const Square to, Move *& move_ptr) const {
    //ピンされた駒がダメな方向へ動く、または自玉が相手の利きに飛び込む違法手に注意しなければならない
    
    //ピンされた駒を先に動かす
    Bitboard pinned_piece(0, 0);

    pinners_.forEach([&](const Square pinner_sq) {
        //pinnerと自玉の間にあるのがpinされている駒
        Bitboard pinned = BETWEEN_BB[pinner_sq][king_sq_[color_]] & occupied_bb_[color_];

        pinned.forEach([&](const Square from) {
            //取る手なのでpinnerを取る手しかダメ
            Bitboard to_bb = controlBB(from, board_[from], occupied_all_) & SQUARE_BB[pinner_sq] & SQUARE_BB[to];

            if (to_bb) {
                pushMove(Move(pinner_sq, from, false, false, board_[from], board_[pinner_sq]), move_ptr);
            }

            //使ったマスを記録しておく
            pinned_piece |= SQUARE_BB[from];
        });
    });

    //玉は別に処理する
    Bitboard king_to_bb = controlBB(king_sq_[color_], board_[king_sq_[color_]], occupied_all_);
    if (king_to_bb & SQUARE_BB[to] && !isThereControl(~color_, to)) { //利きがtoに届いていて、かつそこに相手の利きがない場合動かせる
        *(move_ptr++) = Move(to, king_sq_[color_], false, false, board_[king_sq_[color_]], board_[to]);
    }

    //ピンされた駒と玉は先に処理したので除く
    Bitboard from_bb = attackersTo(color_, to) & ~pinned_piece & ~SQUARE_BB[king_sq_[color_]];
    from_bb.forEach([&](const Square from) {
        pushMove(Move(to, from, false, false, board_[from], board_[to]), move_ptr);
    });
}

void Position::generatePsuedoCaptureMoves(Move *& move_ptr) const {
    //疑似合法手なので玉が取られても良いとする
    Bitboard from_bb = occupied_bb_[color_];
    from_bb.forEach([&](const Square from) {
        //駒を取る手なので利きの先に相手の駒がないといけない
        Bitboard to_bb = controlBB(from, board_[from], occupied_all_) & occupied_bb_[~color_];

        to_bb.forEach([&](const Square to) {
            pushMove(Move(to, from, false, false, board_[from], board_[to]) , move_ptr);
        });
    });
}

void Position::generatePsuedoNonCaptureMoves(Move *& move_ptr) const {
    Bitboard from_bb = occupied_bb_[color_];
    from_bb.forEach([&](const Square from) {
        Bitboard to_bb = controlBB(from, board_[from], occupied_all_) & ~occupied_all_;
        to_bb.forEach([&](const Square to) {
            pushMove(Move(to, from, false, false, board_[from], board_[to]), move_ptr);
        });
    });

    //駒を打つ手
    Bitboard drop_to_bb = (~occupied_all_ & BOARD_BB);
    generateDropMoveBB(drop_to_bb, move_ptr);
}

void Position::generatePsuedoRecaptureMovesTo(const Square to, Move *& move_ptr) const {
    //ピンされた駒と玉は先に処理したので除く
    Bitboard from_bb = attackersTo(color_, to);
    from_bb.forEach([&](const Square from) {
        pushMove(Move(to, from, false, false, board_[from], board_[to]), move_ptr);
    });
}

inline bool Position::isPsuedoLegalMove(const Move move) const {
	//動き方は駒の種類に即していることを前提に、いくつか簡易的なチェックをするだけ

	//移動先が盤内じゃなかったらダメ
	if (!isOnBoard(move.to())) return false;

	//移動先に手番側の駒があったらダメ
	if (pieceToColor(board_[move.to()]) == color_) return false;

	//移動元が玉位置(つまり玉を動かす手)であり、移動先に相手側の利きがあったらダメ
	if (move.from() == king_sq_[color_] && isThereControl(~color_, move.to())) return false;

	return true;
}

inline bool Position::canPromote(Move move) const {
	//打つ手だったらダメ
	if (move.isDrop()) return false;

	//すでに成っている駒を動かす手だったらダメ
	if (board_[move.from()] & PROMOTE) return false;

	//動かす駒が金だったらダメ
	if (kind(board_[move.from()]) == GOLD) return false;

	//動かす駒が玉だったらダメ
	if (kind(board_[move.from()]) == KING) return false;

	//位置関係
	if (color_ == BLACK) {
		return ((Rank1 <= SquareToRank[move.to()] && SquareToRank[move.to()] <= Rank3) || (Rank1 <= SquareToRank[move.from()] && SquareToRank[move.from()] <= Rank3));
	} else if (color_ == WHITE) {
		return ((Rank7 <= SquareToRank[move.to()] && SquareToRank[move.to()] <= Rank9) || (Rank7 <= SquareToRank[move.from()] && SquareToRank[move.from()] <= Rank9));
	}
	return false;
}

void Position::pushMove(const Move move, Move*& move_ptr) const {
	//成る手が可能だったら先に生成
	if (canPromote(move)) {
        *(move_ptr++) = promotiveMove(move);
		switch (kind(board_[move.from()])) {
			//歩、角、飛は成る手しか生成しなくていいだろう
		case PAWN:
		case BISHOP:
		case ROOK:
			return;
			//香、桂は位置によっては成る手しか不可
            //香の2段目への不成は可能だけど意味がないので生成しない
		case LANCE:
		case KNIGHT:
			if (color_ == BLACK && SquareToRank[move.to()] <= Rank2) return;
			if (color_ == WHITE && SquareToRank[move.to()] >= Rank8) return;
			break;
		}
	}
	//成らない手
    *(move_ptr++) = move;
}

void Position::pushCheckMove(const Move move, Move*& move_ptr) {
	//簡易合法判定をする
	if (!isPsuedoLegalMove(move)) return;

	//成る手が可能だったら先に生成
	if (canPromote(move)) {
		Move promotive_move = promotiveMove(move);
		doMove(promotive_move);
        if (isChecked_) {
            *(move_ptr++) = promotive_move;
        }
		undo();
		switch (kind(board_[move.from()])) {
			//歩、角、飛は成る手しか生成しなくていいだろう
		case PAWN:
		case BISHOP:
		case ROOK:
			return;
			//香、桂は位置によっては成る手しか不可
		case LANCE:
			if (color_ == BLACK && SquareToRank[move.to()] == Rank1) return;
			if (color_ == WHITE && SquareToRank[move.to()] == Rank9) return;
			break;
		case KNIGHT:
			if (color_ == BLACK && SquareToRank[move.to()] <= Rank2) return;
			if (color_ == WHITE && SquareToRank[move.to()] >= Rank8) return;
			break;
		}
	}
	//成らない手
	doMove(move);
    if (isChecked_) {
        *(move_ptr++) = move;
    }
	undo();
}

void Position::generateDropMoveBB(const Bitboard& to_bb, Move *& move_ptr) const {
    //歩を打つ手
    //最奥の段は除外する
    if (hand_[color_].num(PAWN) > 0) {
        (to_bb & ~farRank1FromColor(color_)).forEach([&](Square to) {
            if (canDropPawn(to)) {
                *(move_ptr++) = dropMove(to, Piece(PAWN | ColorToFlag[color_]));
            }
        });
    }

    //香車
    //最奥の段は除外する
    if (hand_[color_].num(LANCE) > 0) {
        (to_bb & ~farRank1FromColor(color_)).forEach([&](Square to) {
            *(move_ptr++) = dropMove(to, Piece(LANCE | ColorToFlag[color_]));
        });
    }

    //桂馬
    //奥の2段は除外する
    if (hand_[color_].num(KNIGHT) > 0) {
        (to_bb & ~(farRank1FromColor(color_) | farRank2FromColor(color_))).forEach([&](Square to) {
            *(move_ptr++) = dropMove(to, Piece(KNIGHT | ColorToFlag[color_]));
        });
    }

    //その他
    for (Piece p : { SILVER, GOLD, BISHOP, ROOK }) {
        if (hand_[color_].num(p) > 0) {
            (to_bb).forEach([&](Square to) {
                *(move_ptr++) = dropMove(to, Piece(p | ColorToFlag[color_]));
            });
        }
    }
}

Bitboard Position::attackersTo(const Color c, const Square sq) const {
    //sqへ利きを持っている駒の位置を示すbitboardを返す
    //sqから相手側の駒として利きを求めてみて、その範囲にc側の駒があるなら利きがある
    Bitboard result(0, 0);
    for (Piece p : ColoredPieceList[c]) {
        result |= (controlBB(sq, oppositeColor(p), occupied_all_) & pieces_bb_[p]);
    }
    return result;
}

Bitboard Position::attackersTo(const Color c, const Square sq, const Bitboard & occupied) const {
    //sqへ利きを持っている駒の位置を示すbitboardを返す
    //sqから相手側の駒として利きを求めてみて、その範囲にc側の駒があるなら利きがある
    Bitboard result(0, 0);
    for (Piece p : ColoredPieceList[c]) {
        result |= (controlBB(sq, oppositeColor(p), occupied) & pieces_bb_[p]);
    }
    return result;
}

void Position::computePinners() {
    //pinners
    pinners_ = Bitboard(0, 0);
    //香・角(馬)・飛車(竜)に対してピンのチェック
    for (Piece jumper : ColoredJumpPieceList[~color_]) {
        //自玉からこっちの香・角・飛として利きを駒を最大まで伸ばして
        //そこに相手の香・角(馬)・飛(竜)があったらそれはpinnerになりうる
		//香だったらそれだけ,角飛だったらなったものについて存在位置を取る
		Bitboard jumper_bb = (kind(jumper) == LANCE ? pieces_bb_[jumper] : pieces_bb_[jumper] | pieces_bb_[promote(jumper)]);
        Bitboard pinner_candidate = controlBB(king_sq_[color_], oppositeColor(jumper), Bitboard(0, 0)) & jumper_bb;

        //各pinnerの候補についてking_sq_との間を見ていく
        pinner_candidate.forEach([&](const Square jump_piece_sq) {
            Bitboard between = BETWEEN_BB[king_sq_[color_]][jump_piece_sq] & occupied_all_;
            if (between.pop_count() == 1 && (between & occupied_bb_[color_])) { //betweenに駒が1個だけかつ駒が手番側のものだったらピンされている
                                                                                //ピンしている駒を記録しておけばピンされた駒は自玉との間でbetween見れば復元できる
                pinners_ |= SQUARE_BB[jump_piece_sq];
            }
        });
    }
}

std::vector<Move> Position::generateAllMoves() const {
    Move move_buf[MAX_MOVE_LIST_SIZE];
    Move* move_ptr = move_buf;
	//手番側に王手がかかっていたら逃れる手だけを生成
    if (isChecked_) {
        generateEvasionMovesBB(move_ptr);
    } else {
        generateCaptureMovesBB(move_ptr);
        generateNonCaptureMovesBB(move_ptr);
    }

    std::vector<Move> move_list;
    for (move_ptr--; move_buf <= move_ptr; move_ptr--) {
        move_list.push_back(*move_ptr);
    }
	return move_list;
}

void Position::generateCheckMoveBB(Move *& move_ptr) const {
    auto start_ptr = move_ptr;

    if (isChecked_) {
        //自玉に王手がかかっていたら逆王手するしかない
        //王手を解除する手はさほど多くないだろうから全部生成して動かしてみて王手になってるものだけを追加する
        Move buf[MAX_MOVE_LIST_SIZE];
        Move* tmp_ptr = buf;
        generateEvasionMovesBB(tmp_ptr);
        for (Move* itr = buf; itr < tmp_ptr; itr++) {
            Position p(*this);
            //doMove内で評価値を差分計算しているけど使わないので、抜いたdoMoveが欲しいなぁ
            //あるいはMoveを与えたらそれが王手になっているかどうか確認する関数でもいいのか.むしろそのほうが普通かな
            p.doMove(*itr);
            if (p.isKingChecked()) {
                *(move_ptr++) = *itr;
            }
            p.undo();
        }
        return;
    }

    Bitboard moved_pieces(0, 0);
    //pinnerを先に処理する
    for (Piece jumper : ColoredJumpPieceList[color_]) {
        //敵玉から相手の香・角・飛として利きを駒を最大まで伸ばして
        //そこに相手の香・角(馬)・飛(竜)があったらそれはpinnerになりうる
		//香だったらそれだけ,角飛だったらなったものについて存在位置を取る
		Bitboard jumper_bb = (kind(jumper) == LANCE ? pieces_bb_[jumper] : pieces_bb_[jumper] | pieces_bb_[promote(jumper)]);

		Bitboard pinner_candidate = controlBB(king_sq_[~color_], oppositeColor(jumper), Bitboard(0, 0)) & jumper_bb;

        //先に処理した駒を記録しておく
        moved_pieces |= pinner_candidate;

        //各pinnerの候補についてking_sq_との間を見ていく
        pinner_candidate.forEach([&](const Square jump_piece_sq) {
            Bitboard between = BETWEEN_BB[king_sq_[color_]][jump_piece_sq] & occupied_all_;
            if (between.pop_count() == 1) { //betweenに駒が1個だけだったらなんらかの王手がかけられるかも
                if (between & occupied_bb_[color_]) { //間にあるのが自分の駒
                    //開き王手or普通に押していく王手ができる可能性がある
                    //間にある駒が同じ方向への飛び駒である可能性はない(それだったら相手の玉を取れている)
                    const Square from = between.pop();
                    const Piece piece = board_[from];

                    //間にあった駒も動かすので記録しておく
                    moved_pieces |= SQUARE_BB[from];

                    //開き王手(jump_pieceと相手玉のbetween以外への移動)
                    //両王手を含む
                    if (kind(piece) == KING) {
                        //間にある駒が自玉だったら自殺手に気を付ける
                        Bitboard to_bb = controlBB(from, piece, occupied_all_) & ~BETWEEN_BB[jump_piece_sq][king_sq_[~color_]];
                        to_bb.forEach([&](const Square to) {
                            if (!attackersTo(~color_, to, occupied_all_ & ~SQUARE_BB[from])) {
                                pushMove(Move(to, from, false, false, piece, board_[to]), move_ptr);
                            }
                        });
                        return;
                    } else {
                        Bitboard to_bb = controlBB(from, piece, occupied_all_) & ~BETWEEN_BB[jump_piece_sq][king_sq_[~color_]];
                        to_bb.forEach([&](const Square to) {
                            pushMove(Move(to, from, false, false, piece, board_[to]), move_ptr);
                        });
                    }

                    //between内で移動する王手
                    //飛車先の歩を進める王手みたいなイメージ
                    //不成
                    //相手玉の位置から進める駒の手番を逆にして利きを求め、そこへ利きがあり、between内であり、自駒がなければそこへ行く手が王手
                    Bitboard non_promotion_to_bb = controlBB(king_sq_[~color_], oppositeColor(piece), occupied_all_) 
                        & controlBB(from, piece, occupied_all_) & BETWEEN_BB[jump_piece_sq][king_sq_[~color_]] & ~occupied_bb_[color_];
                    non_promotion_to_bb.forEach([&](const Square to) {
                        *(move_ptr++) = Move(to, from, false, false, piece, board_[to]);
                    });

                    //成
                    if (!(piece & PROMOTE) && kind(piece) != GOLD) {
                        Bitboard promotion_to_bb = controlBB(king_sq_[~color_], oppositeColor(piece) | PROMOTE, occupied_all_)
                            & controlBB(from, piece, occupied_all_) & BETWEEN_BB[jump_piece_sq][king_sq_[~color_]] & ~occupied_bb_[color_];
                        non_promotion_to_bb.forEach([&](const Square to) {
                            if ((SQUARE_BB[from] | SQUARE_BB[to]) | PROMOTION_ZONE_BB[color_]) {
                                *(move_ptr++) = Move(to, from, false, true, piece, board_[to]);
                            }
                        });
                    }
                } else if (between & occupied_bb_[color_]) { //間にあるのが相手の駒
                    //その駒を取る王手がありえる
                    const Square to = between.pop();
                    pushMove(Move(to, jump_piece_sq, false, false, board_[jump_piece_sq], board_[to]), move_ptr);
                }
            }
        });
    }

    //自駒の中からすでに動かした駒は排除
    Bitboard from_bb = occupied_bb_[color_] & ~moved_pieces;
    from_bb.forEach([&](const Square from) {
        const Piece piece = board_[from];
        if (kind(piece) == ROOK || kind(piece) == BISHOP || (kind(piece) == LANCE && !(piece & PROMOTE))) { //飛び駒
            //不成の王手
            //相手玉から相手の駒として利きを求め、それと今見ている駒の利きが被ったところ、かつそこに自駒がない位置へ行く手が王手
            Bitboard non_promotion_to_bb = controlBB(king_sq_[~color_], oppositeColor(piece), occupied_all_) & controlBB(from, piece, occupied_all_) & ~occupied_bb_[color_];
            non_promotion_to_bb.forEach([&](const Square to) {
                *(move_ptr++) = Move(to, from, false, false, board_[from], board_[to]);
            });

            if (!(piece & PROMOTE)) {
                //まだ成ってない駒だったら成の王手
                Bitboard promotion_to_bb = controlBB(king_sq_[~color_], oppositeColor(piece), occupied_all_) & controlBB(from, piece, occupied_all_) & ~occupied_bb_[color_];
                promotion_to_bb.forEach([&](const Square to) {
                    if ((SQUARE_BB[to] | SQUARE_BB[from]) & PROMOTION_ZONE_BB[color_]) {
                        *(move_ptr++) = Move(to, from, false, true, board_[from], board_[to]);
                    }
                });
            }

        } else { //飛び駒でない
            //不成の王手
            //相手玉から相手の駒として利きを求め、それと今見ている駒の利きが被ったところ、かつそこに自駒がない位置へ行く手が王手
            Bitboard non_promotion_to_bb = controlBB(king_sq_[~color_], oppositeColor(piece), occupied_all_) & controlBB(from, piece, occupied_all_) & ~occupied_bb_[color_];
            non_promotion_to_bb.forEach([&](const Square to) {
                *(move_ptr++) = Move(to, from, false, false, board_[from], board_[to]);
            });

            if (!(piece & PROMOTE) && (kind(piece) != GOLD) &&(kind(piece) != KING)) {
                //まだ成ってない駒かつ金でないかつ玉でない場合、成りながらの王手がありえる
                //相手玉の位置から成った後の駒を相手の駒として利きを求めて、そこへ成らずにいけるかつ、自駒がそこにない場合そこへ成る手が王手
                Bitboard promotion_to_bb = controlBB(king_sq_[~color_], oppositeColor(piece) | PROMOTE, occupied_all_) & controlBB(from, piece, occupied_all_) & ~occupied_bb_[color_];
                promotion_to_bb.forEach([&](const Square to) {
                    if ((SQUARE_BB[to] | SQUARE_BB[from]) & PROMOTION_ZONE_BB[color_]) {
                        *(move_ptr++) = Move(to, from, false, true, board_[from], board_[to]);
                    }
                });
            }
        }
    });

    //駒打ちの王手
    for (Piece piece : { PAWN, LANCE, KNIGHT, SILVER, GOLD, BISHOP, ROOK }) {
        if (hand_[color_].num(piece)) {
            Bitboard to_bb = controlBB(king_sq_[~color_], coloredPiece(~color_, piece), occupied_all_) & ~occupied_all_;
            to_bb.forEach([&](const Square to) {
                if (piece == PAWN) {
                    //駒が歩だったら二歩と打ち歩詰めの確認
                    if (canDropPawn(to)) {
                        *(move_ptr++) = Move(to, WALL00, true, false, coloredPiece(color_, piece), board_[to]);
                    }
                } else {
                    *(move_ptr++) = Move(to, WALL00, true, false, coloredPiece(color_, piece), board_[to]);
                }
            });
        }
    }

#if DEBUG
    print();
    std::cout << "この局面の王手" << std::endl;
    for (; start_ptr < move_ptr; start_ptr++) {
        start_ptr->print();
        Position p(*this);
        p.doMove(*start_ptr);
        if (!p.isKingChecked()) {
            //王手になっていないものがあったらおかしい
            assert(false);
        }
        p.undo();
    }

    std::cout << "以下確認" << std::endl;
    std::vector<Move> check_moves;
    std::vector<Move> all_moves = generateAllMoves();
    for (Move m : all_moves) {
        //動かしてみて王手がかかる手だけ加える
        //TODO:最悪な実装なのでなんとか高速化する.せめて評価値を差分計算しないdoMoveを作ってそっちを利用する
        Position p(*this);
        p.doMove(m);
        if (p.isKingChecked()) {
            check_moves.push_back(m);
        }
        p.undo();
    }
    std::cout << "check_moves.size() = " << check_moves.size() << std::endl;
#endif
}

bool Position::isRepeating(Score& score) {
    //千日手or連続王手の千日手だったらtrueを返してscoreに適切な値を入れる(技巧と似た実装)

    if (turn_number_ <= 4) {
        return false;
    }

    //現局面のindex
    auto index = stack_.size();

    if (board_hash_ == stack_[index - 4].board_hash //局面が一致
        && hand_hash_ == stack_[index - 4].hand_hash //手駒も一致
        ) { //4手前と局面が一致する
        //連続王手かどうか
        if (isChecked_ && stack_[index - 2].isChecked) {
            score = MAX_SCORE;
        } else {
            score = DRAW_SCORE;
        }
        return true;
    }
    return false;
}