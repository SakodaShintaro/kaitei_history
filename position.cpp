#include"position.hpp"
#include"piece.hpp"
#include"move.hpp"
#include"common.hpp"
#include<iostream>
#include<cstdio>
#include<ctime>
#include<bitset>
#include<cassert>
#include<iterator>
#include<intrin.h>

Position::Position()
{
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

	//利き
	initControl();

	//玉の位置
	king_sq_[BLACK] = SQ59;
	king_sq_[WHITE] = SQ51;

	//局面の評価値
	score_ = initScore();

	//ハッシュ値
	hashInit();
	for (int r = Rank1; r <= Rank9; r++) {
		for (int f = File1; f <= File9; f++) {
			Square pos = FRToSquare[f][r];
			hash_val_ ^= HashSeed[board_[pos]][pos];
		}
	}
	for (Piece piece = PAWN; piece <= ROOK; piece++) {
		hash_val_ ^= HandHashSeed[BLACK][piece][hand_[BLACK].num(piece)];
		hash_val_ ^= HandHashSeed[WHITE][piece][hand_[WHITE].num(piece)];
	}
	hash_val_ &= ~1; //これで1bit目が0になる(先手番を表す)

	//出現回数
	num_occurrences[hash_val_]++;
}

void Position::print() const
{
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
	printf("玉利評価値:%d\n", control_score_);
	printf("利数評価値:%d\n", control_num_score_);
	printf("合計評価値:%d\n", score_);

	//利き
	printf("先手の利き\n");
	printf("９ ８ ７ ６ ５ ４ ３ ２ １ \n");
	printf("---------------------------\n");
	for (int r = Rank1; r <= Rank9; r++) {
		for (int f = File9; f >= File1; f--) {
			printf("%2d ", control_num_[BLACK][FRToSquare[f][r]]);
		}
		printf("|%d\n", r);
	}
	printf("後手の利き\n");
	printf("９ ８ ７ ６ ５ ４ ３ ２ １ \n");
	printf("---------------------------\n");
	for (int r = Rank1; r <= Rank9; r++) {
		for (int f = File9; f >= File1; f--) {
			printf("%2d ", control_num_[WHITE][FRToSquare[f][r]]);
		}
		printf("|%d\n", r);
	}
	printf("ハッシュ値:%lld\n", hash_val_);

	//局面の登場回数
	//printf("同一局面の登場回数:%d\n", num_occurrences[hash_val_]);

	//printCheckMoves();
	//printCaptureMoves();
	//printControlDir();
	//printAllMoves();
}

void Position::printCheckMoves()
{
	std::vector<Move> check_moves = generateCheckMoves();
	printf("可能な王手の数:%zu\n", check_moves.size());
	for (Move move : check_moves) move.print();
}

void Position::printControlDir()
{
	for (int r = Rank1; r <= Rank9; r++) {
		for (int f = File1; f <= File9; f++) {
			Square sq = FRToSquare[f][r];
			std::cout << "(" << f << "," << r << ")" << std::endl;
			std::cout << "----SRQPONMLKJIHGFEDCBA987654321" << std::endl;
			std::cout << static_cast<std::bitset<32>>(control_dir_[BLACK][sq]) << std::endl;
			std::cout << static_cast<std::bitset<32>>(control_dir_[WHITE][sq]) << std::endl;
		}
	}
}

void Position::printAllMoves()
{
	std::vector<Move> moves = generateAllMoves();
	printf("全合法手の数:%zu\n", moves.size());
	for (Move move : moves) move.print();
}

void Position::printCaptureMoves()
{
	std::vector<Move> moves;
	addCaptureMoves(moves);
	printf("駒を取る手の数:%zu\n", moves.size());
	for (Move move : moves) move.print();
}

void Position::printNonCaptureMoves()
{
	std::vector<Move> moves;
	addNonCaptureMoves(moves);
	printf("駒を取らない手の数:%zu\n", moves.size());
	for (Move move : moves) move.print();
}

void Position::printHistory() const
{
	printf("print history\n");
	for (Move move : kifu_) move.print();
	printf("\n");
}

void Position::doMove(Move move)
{
	//undoで戻すために各種メンバ変数を設定しておく
	if (move.isDrop()) { //subjectは基本的に手番付きで入っているはずだが、念のため手番を入れておく
		move = dropMove(move.to(), (color_ == BLACK ? toBlack(move.subject()) : toWhite(move.subject())));
	} else {
		move = fullMove(move.to(), move.from(), false, move.isPromote(), board_[move.from()], board_[move.to()]);
	}

	//実際に動かす
	if (move.isDrop()) { //持ち駒を打つ手

		//持ち駒を減らす
		hand_[color_].sub(kind(move.subject()));

		//移動先にsubjectを設置
		board_[move.to()] = move.subject();

		//ハッシュ値の更新
		//打つ前のHandHashとXORして消す
		hash_val_ ^= HandHashSeed[color_][move.subject()][hand_[color_].num(kind(move.subject())) + 1];
		//打った後のHandHashとXOR
		hash_val_ ^= HandHashSeed[color_][move.subject()][hand_[color_].num(kind(move.subject()))];
		//打った後の分をXOR
		hash_val_ ^= HashSeed[move.subject()][move.to()];

	} else { //盤上の駒を動かす手
		
		//移動する駒を消す
		deleteControl(move.from());
		board_[move.from()] = EMPTY;

		//取った駒があるならその駒を消し、持ち駒を増やす
		if (move.capture() != EMPTY) {
			deleteControl(move.to());
			board_[move.to()] = EMPTY;
			hand_[color_].add(kind(move.capture()));

			//ハッシュ値の更新
			//取った駒分のハッシュをXOR
			hash_val_ ^= HashSeed[move.capture()][move.to()];
			//増える前の持ち駒の分をXORして消す
			hash_val_ ^= HandHashSeed[color_][kind(move.capture())][hand_[color_].num(kind(move.capture())) - 1];
			//増えた後の持ち駒の分XOR
			hash_val_ ^= HandHashSeed[color_][kind(move.capture())][hand_[color_].num(kind(move.capture()))];
		}

		//成る手ならsubjectに成りのフラグを立てて,そうでないならsubjectをそのまま移動先に設置
		if (move.isPromote()) board_[move.to()] = promote(move.subject());
		else board_[move.to()] = move.subject();

		//ハッシュ値の更新
		//移動前の分をXORして消す
		hash_val_ ^= HashSeed[move.subject()][move.from()];
		//移動後の分をXOR
		hash_val_ ^= HashSeed[move.subject()][move.to()];
	}

	//移動した駒の利きを増やす
	addControl(move.to());

	//玉を動かす手ならblack_king_pos,white_king_posに反映
	if (kind(move.subject()) == KING) king_sq_[color_] = move.to();

	//手番の更新
	color_ = ~color_;

	//手数の更新
	turn_number_++;

	//棋譜に指し手を追加
	kifu_.push_back(move);

	//hashの手番要素を更新
	//1bit目を0にする
	hash_val_ &= ~1;
	//手番が先手だったら1bitは0のまま,後手だったら1bit目は1になる
	hash_val_ |= color_;

	//出現回数を増やす
	//num_occurrences[hash_val_]++;
}

void Position::undo()
{
	const Move last_move = kifu_.back();
	kifu_.pop_back();

	//出現回数を減らす
	//num_occurrences[hash_val_]--;

	//手番を戻す(このタイミングでいいかな?)
	color_ = ~color_;

	//動かした駒を消す
	deleteControl(last_move.to());
	board_[last_move.to()] = EMPTY;

	//盤の状態を戻す
	if (last_move.isDrop()) { //打つ手

		//持ち駒を増やす
		hand_[color_].add(kind(last_move.subject()));

		//ハッシュ値の巻き戻し
		//戻す前のHandHashとXOR
		hash_val_ ^= HandHashSeed[color_][last_move.subject()][hand_[color_].num(kind(last_move.subject())) - 1];
		//戻す前の分をXORして消す
		hash_val_ ^= HashSeed[last_move.subject()][last_move.to()];
		//戻した後のHandHashとXOR
		hash_val_ ^= HandHashSeed[color_][last_move.subject()][hand_[color_].num(kind(last_move.subject()))];

	} else { //盤上の駒を動かす手
		
		//toに取った駒を戻し、持ち駒を減らす
		if (last_move.capture() != EMPTY) {
			board_[last_move.to()] = last_move.capture();
			addControl(last_move.to());
			hand_[color_].sub(kind(last_move.capture()));

			//ハッシュ値の巻き戻し
			//取る前の分のハッシュをXOR
			hash_val_ ^= HashSeed[last_move.capture()][last_move.to()];
			//増える前の持ち駒の分
			hash_val_ ^= HandHashSeed[color_][last_move.capture() & PIECE_KIND_MASK][hand_[color_].num(kind(last_move.capture()))];
			//増えた後の持ち駒の分XORして消す
			hash_val_ ^= HandHashSeed[color_][last_move.capture() & PIECE_KIND_MASK][hand_[color_].num(kind(last_move.capture())) + 1];
		}

		//動いた駒をfromに戻す
		board_[last_move.from()] = last_move.subject();
		//戻した駒の利きを足す
		addControl(last_move.from());

		//ハッシュ値の巻き戻し
		//移動前の分をXOR
		hash_val_ ^= HashSeed[last_move.subject()][last_move.from()];
		//移動後の分をXORして消す
		hash_val_ ^= HashSeed[last_move.subject()][last_move.to()];
	}

	//玉を動かす手ならking_sq_に反映
	if (kind(last_move.subject()) == KING) king_sq_[color_] = last_move.from();

	//ハッシュの更新(手番分)
	//一番右のbitを0にする
	hash_val_ &= ~1;
	//一番右のbitが先手番だったら0のまま、後手番だったら1になる
	hash_val_ |= color_;

	//手数
	turn_number_--;
}

bool Position::isLegalMove(const Move move)
{
	//違法の場合だけ早くfalseで返す.合法手は一番最後の行でtrueが返る

	//NULL_MOVEだけは先に判定するか……
	if (move == NULL_MOVE) {
		return false;
	}

	//打つ手と動かす手両方に共通するもの
	//移動先のチェック
	if (!isOnBoard(move.to())) {
		std::cout << "移動先が盤上ではありません" << std::endl;
		return false;
	}
	if (color_ == BLACK && pieceToColor(board_[move.to()]) == BLACK) {
		std::cout << "先手の移動先に先手の駒があります" << std::endl;
		return false;
	}
	if (color_ == WHITE && pieceToColor(board_[move.to()]) == WHITE) {
		std::cout << "後手の移動先に後手の駒があります" << std::endl;
		return false;
	}

	//動かす駒があるかチェック
	if (board_[move.from()] == EMPTY) {
		std::cout << "無を動かしています" << std::endl;
		return false;
	}

	//手番と動かす駒の対応チェック
	if (color_ == BLACK && (pieceToColor(move.subject()) == WHITE)) {
		std::cout << "先手の指し手で後手の駒を動かしています" << std::endl;
		return false;
	}
	if (color_ == WHITE && (pieceToColor(move.subject()) == BLACK)) {
		std::cout << "後手の指し手で先手の駒を動かしています" << std::endl;
		return false;
	}

	//成りのチェック
	//CSA形式棋譜をパーサする際に成判定がうまくできないのでここは削除
	//if (move.isPromote() && color_ == BLACK) {
	//	if (SquareToRank[move.to()] > Rank3 && SquareToRank[move.from()] > Rank3) {
	//		std::cout << "自陣で成っています" << std::endl;
	//		return false;
	//	}
	//}
	//if (move.isPromote() && color_ == WHITE) {
	//	if (SquareToRank[move.to()] < Rank7 && SquareToRank[move.from()] < Rank7) {
	//		std::cout << "自陣で成っています" << std::endl;
	//		return false;
	//	}
	//}
	if (move.isDrop()) { //駒を打つ手特有の判定
		//打つ先が空になってるか
		if (board_[move.to()] != EMPTY) {
			std::cout << "打つ先に駒があります" << std::endl;
			return false;
		}
		//二歩になっていないか
		if (move.subject() == BLACK_PAWN && !canDropPawn<BLACK>(SquareToFile[move.to()])) {
			std::cout << "二歩です" << std::endl;
			return false;
		}
		if (move.subject() == WHITE_PAWN && !canDropPawn<WHITE>(SquareToFile[move.to()])) {
			std::cout << "二歩です" << std::endl;
			return false;
		}
		//歩を最奥段に打っていないか
		if (move.subject() == BLACK_PAWN && SquareToRank[move.to()] == Rank1) {
			std::cout << "一段目に歩を打っています" << std::endl;
			return false;
		}
		if (move.subject() == WHITE_PAWN && SquareToRank[move.to()] == Rank9) {
			std::cout << "九段目に歩を打っています" << std::endl;
			return false;
		}
		//香を最奥段に打っていないか
		if (move.subject() == BLACK_LANCE && SquareToRank[move.to()] == Rank1) {
			std::cout << "一段目に香を打っています" << std::endl;
			return false;
		}
		if (move.subject() == WHITE_LANCE && SquareToRank[move.to()] == Rank9) {
			std::cout << "九段目に香を打っています" << std::endl;
			return false;
		}
		//桂を最奥段に打っていないか
		if (move.subject() == BLACK_KNIGHT && SquareToRank[move.to()] == Rank1) {
			std::cout << "一段目に桂を打っています" << std::endl;
			return false;
		}
		if (move.subject() == WHITE_KNIGHT && SquareToRank[move.to()] == Rank9) {
			std::cout << "九段目に桂を打っています" << std::endl;
			return false;
		}
		//桂を奥から二段目に打っていないか
		if (move.subject() == BLACK_KNIGHT && SquareToRank[move.to()] == Rank2) {
			std::cout << "二段目に香を打っています" << std::endl;
			return false;
		}
		if (move.subject() == WHITE_KNIGHT && SquareToRank[move.to()] == Rank8) {
			std::cout << "八段目に香を打っています" << std::endl;
			return false;
		}
	} else { //盤上の駒を動かす手の判定
		//各駒に合わせた動きになっているか
		bool flag = false;
		for (unsigned int i = 0; i < CanMove[board_[move.from()]].size(); i++) {
			if (move.to() == move.from() + CanMove[board_[move.from()]][i]) flag = true;
		}
		for (unsigned int i = 0; i < CanJump[board_[move.from()]].size(); i++) {
			for (Square sq = move.from() + CanJump[board_[move.from()]][i]; board_[sq] != WALL; sq = sq + CanJump[board_[move.from()]][i]) {
				if (move.to() == sq) flag = true;
			}
		}
		if (!flag) {
			std::cout << PieceToStr[board_[move.from()]] << "の違法な動き" << std::endl;
			return false;
		}
	}

	//王手放置
	if (kind(board_[move.from()]) == KING && control_num_[~color_][move.to()] > 0) {
		std::cout << "王手放置" << std::endl;
		return false;
	}

	//玉を取る手がなぜか発生する場合
	if (kind(board_[move.to()]) == KING) {
		std::cout << "玉を取る手" << std::endl;
		return false;
	}
	return true;
}

bool Position::isLegalControl()
{
	//盤面を走査しておかしくなってないか確認
	unsigned int copy[ColorNum][SquareNum];
	for (Square sq : SquareList) {
		copy[BLACK][sq] = control_num_[BLACK][sq];
		copy[WHITE][sq] = control_num_[WHITE][sq];
	}
	initControl();
	for (Square sq : SquareList) {
		if (copy[BLACK][sq] != control_num_[BLACK][sq]) {
			//おかしくなっていたらいろいろ表示
			for (Move history : kifu_) history.print();
			print();
			//おかしい利き表示
			std::cout << "違うマス:" << sq << std::endl;
			printf("おかしい先手利き\n");
			printf("９ ８ ７ ６ ５ ４ ３ ２ １\n");
			printf("---------------------------\n");
			for (int r = Rank1; r <= Rank9; r++) {
				for (int f = File9; f >= File1; f--) {
					printf("%2d ", copy[BLACK][FRToSquare[f][r]]);
				}
				printf("|%d\n", r);
			}
			printf("おかしい後手の利き\n");
			printf("９ ８ ７ ６ ５ ４ ３ ２ １\n");
			printf("---------------------------\n");
			for (int r = Rank1; r <= Rank9; r++) {
				for (int f = File9; f >= File1; f--) {
					printf("%2d ", copy[WHITE][FRToSquare[f][r]]);
				}
				printf("|%d\n", r);
			}
			assert(false);
		}
		if (copy[WHITE][sq] != control_num_[WHITE][sq]) {
			//おかしくなっていたらいろいろ表示
			for (Move history : kifu_) history.print();
			print();
			//おかしい利き表示
			std::cout << "違うマス:" << sq << std::endl;
			printf("おかしい先手利き\n");
			printf(" ９ ８ ７ ６ ５ ４ ３ ２ １\n");
			printf("---------------------------\n");
			for (int r = Rank1; r <= Rank9; r++) {
				for (int f = File9; f >= File1; f--) {
					printf("%2d ", copy[BLACK][FRToSquare[f][r]]);
				}
				printf("|%d\n", r);
			}
			printf("おかしい後手の利き\n");
			printf(" ９ ８ ７ ６ ５ ４ ３ ２ １\n");
			printf("---------------------------\n");
			for (int r = Rank1; r <= Rank9; r++) {
				for (int f = File9; f >= File1; f--) {
					printf("%2d ", copy[WHITE][FRToSquare[f][r]]);
				}
				printf("|%d\n", r);
			}
			assert(false);
		}
	}
	return false;
}

template<Color color>
inline bool Position::canDropPawn(const File f)
{
	if (color == BLACK) {
		for (int r = Rank1; r <= Rank9; r++)
			if (board_[FRToSquare[f][r]] == BLACK_PAWN) return false;
	} else {
		for (int r = Rank1; r <= Rank9; r++)
			if (board_[FRToSquare[f][r]] == WHITE_PAWN) return false;
	}
	return true;
}

void Position::load_sfen(std::string sfen)
{
	std::printf("start load_sfen\n");
	//初期化
	for (int i = 0; i < SquareNum; i++) board_[i] = WALL;
	for (Square sq : SquareList) board_[sq] = EMPTY;

	//テーブル用意しておいたほうがシュッと書ける
	std::map<char, Piece> CharToPiece = {
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
	i++;

	//持ち駒
	hand_[BLACK].clear();
	hand_[WHITE].clear();
	while (sfen[i] != ' ') {
		switch (sfen[i]) {
		case 'P':
			if ('1' <= sfen[i + 1] && sfen[i + 1] <= '9') {
				if ('1' <= sfen[i + 2] && sfen[i + 2] <= '9') {
					hand_[BLACK].set(PAWN, 10 * (sfen[i + 1] - '0') + (sfen[i + 2] - '0'));
					i += 2;
				} else {
					hand_[BLACK].set(PAWN, sfen[i++] - '0');
				}
			} else {
				hand_[BLACK].set(PAWN, 1);
			}
			break;
		case 'L':
		case 'N':
		case 'S':
		case 'G':
			if ('2' <= sfen[i + 1] && sfen[i + 1] <= '4') {
				hand_[BLACK].set(kind(CharToPiece[sfen[i]]), sfen[i++] - '0');
			} else {
				hand_[BLACK].set(kind(CharToPiece[sfen[i]]), 1);
			}
			break;
		case 'B':
		case 'R':
			if (sfen[i + 1] == '2') {
				hand_[BLACK].set(kind(CharToPiece[sfen[i]]), 2);
			} else {
				hand_[BLACK].set(kind(CharToPiece[sfen[i]]), 1);
			}
			break;
		case 'p':
			if ('1' <= sfen[i + 1] && sfen[i + 1] <= '9') {
				if ('1' <= sfen[i + 2] && sfen[i + 2] <= '9') {
					hand_[WHITE].set(PAWN, 10 * (sfen[i + 1] - '0') + (sfen[i + 2] - '0'));
					i += 2;
				} else {
					hand_[WHITE].set(PAWN, sfen[i++] - '0');
				}
			} else {
				hand_[WHITE].set(PAWN, 1);
			}
			break;
		case 'l':
		case 'n':
		case 's':
		case 'g':
			if ('2' <= sfen[i + 1] && sfen[i + 1] <= '4') {
				hand_[WHITE].set(kind(CharToPiece[sfen[i]]), sfen[i++] - '0');
			} else {
				hand_[WHITE].set(kind(CharToPiece[sfen[i]]), 1);
			}
			break;
		case 'b':
		case 'r':
			if (sfen[i + 1] == '2') {
				hand_[WHITE].set(kind(CharToPiece[sfen[i]]), 2);
			} else {
				hand_[WHITE].set(kind(CharToPiece[sfen[i]]), 1);
			}
			break;
		}
		i++;
	}

	//手数
	turn_number_ = 0;

	//利き
	initControl();

	//局面の評価値
	score_ = initScore();

	//局面の登場回数初期化
	num_occurrences.clear();
}

void Position::hashInit()
{
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

std::vector<Move> Position::generateCheckMoves()
{
	std::vector<Move> check_moves;
	for (Square sq : SquareList) {
		Piece subject = board_[sq];
		//隣接および桂馬の動き
		for (unsigned int i = 0; i < CanMove[subject].size(); i++) {
			pushCheckMove(basicMove(sq + CanMove[subject][i], sq), check_moves);
		}
		//飛ぶ動き
		for (unsigned int i = 0; i < CanJump[subject].size(); i++) {
			for (Square to = sq + CanJump[subject][i]; board_[to] != WALL; to = to + CanJump[subject][i]) {
				pushCheckMove(basicMove(to, sq), check_moves);
			}
		}
	}

	for (Square to : SquareList) {
		if (board_[to] == EMPTY && color_ == BLACK) {
			if (SquareToRank[to] >= Rank2) {
				if (canDropPawn<BLACK>(SquareToFile[to]))
					pushCheckMove(dropMove(to, PAWN), check_moves);
				pushCheckMove(dropMove(to, LANCE), check_moves);
			}
			if (SquareToRank[to] >= Rank3) {
				pushCheckMove(dropMove(to, KNIGHT), check_moves);
			}
			for (Piece p = SILVER; p <= ROOK; p++) {
				if (hand_[BLACK].num(p) > 0) {
					pushCheckMove(dropMove(to, p), check_moves);
				}
			}
		}
		if (board_[to] == EMPTY && color_ == WHITE) {
			if (SquareToRank[to] <= Rank8) {
				if (canDropPawn<WHITE>(static_cast<File>(SquareToFile[to])))
					pushCheckMove(dropMove(to, PAWN), check_moves);
				pushCheckMove(dropMove(to, LANCE), check_moves);
			}
			if (SquareToRank[to] <= Rank7) {
				pushCheckMove(dropMove(to, KNIGHT), check_moves);
			}
			for (Piece p = SILVER; p <= ROOK; p++) {
				if (hand_[WHITE].num(p) > 0) {
					pushCheckMove(dropMove(to, p), check_moves);
				}
			}
		}
	}
	return check_moves;
}

Move Position::generateCheapestMoveTo(Square to)
{
	//交換値の決定に使うので安い駒から順番に考えて可能だったらすぐ返す
	if (color_ == BLACK) {
		//歩
		if (board_[to + D] == BLACK_PAWN) {
			return basicMove(to, to + D);
		}
		//香
		for (Square from = to + D; board_[from] != WALL; from = from + D) {
			//香があればよい
			if (board_[from] == BLACK_LANCE) {
				return basicMove(to, from);
			} else if (board_[from] != EMPTY) { //ほかの駒があったらダメ
				break;
			}
		}
		//桂
		if (board_[to + LDD] == BLACK_KNIGHT) {
			return basicMove(to, to + LDD);
		} else if (board_[to + RDD] == BLACK_KNIGHT) {
			return basicMove(to, to + RDD);
		}
		//銀
		for (unsigned int i = 0; i < CanMove[WHITE_SILVER].size(); i++) {
			if (board_[to + CanMove[WHITE_SILVER][i]] == BLACK_SILVER)
				return basicMove(to, to + CanMove[WHITE_SILVER][i]);
		}
		//金
		for (unsigned int i = 0; i < CanMove[WHITE_GOLD].size(); i++) {
			switch (board_[to + CanMove[WHITE_GOLD][i]]) {
			case BLACK_GOLD:
			case BLACK_PAWN_PROMOTE:
			case BLACK_LANCE_PROMOTE:
			case BLACK_KNIGHT_PROMOTE:
			case BLACK_SILVER_PROMOTE:
				return basicMove(to, to + CanMove[WHITE_GOLD][i]);
			}
		}
		//馬
		for (unsigned int i = 0; i < CanMove[WHITE_BISHOP_PROMOTE].size(); i++) {
			if (board_[to + CanMove[WHITE_BISHOP_PROMOTE][i]] == BLACK_BISHOP_PROMOTE)
				return basicMove(to, to + CanMove[WHITE_BISHOP_PROMOTE][i]);
		}
		//角と馬
		for (unsigned int i = 0; i < CanJump[WHITE_BISHOP].size(); i++) {
			for (Square from = to + CanJump[WHITE_BISHOP][i]; board_[from] != WALL; from = from + CanJump[WHITE_BISHOP][i]) {
				if (board_[from] == BLACK_BISHOP || board_[from] == BLACK_BISHOP_PROMOTE) {
					return basicMove(to, from);
				} else if (board_[from] != EMPTY) break;
			}
		}
		//竜
		for (unsigned int i = 0; i < CanMove[WHITE_ROOK_PROMOTE].size(); i++) {
			if (board_[to + CanMove[WHITE_ROOK_PROMOTE][i]] == BLACK_ROOK_PROMOTE)
				return basicMove(to, to + CanMove[WHITE_ROOK_PROMOTE][i]);
		}
		//飛車と竜
		for (unsigned int i = 0; i < CanJump[WHITE_ROOK].size(); i++) {
			for (Square from = to + CanJump[WHITE_ROOK][i]; board_[from] != WALL; from = from + CanJump[WHITE_ROOK][i]) {
				if (board_[from] == BLACK_ROOK || board_[from] == BLACK_ROOK_PROMOTE) {
					return basicMove(to, from);
				} else if (board_[from] != EMPTY) break;
			}
		}
		//玉
		for (unsigned int i = 0; i < CanMove[WHITE_KING].size(); i++) {
			if (board_[to + CanMove[WHITE_KING][i]] == BLACK_KING && control_num_[~color_][to] == 0)
				return basicMove(to, to + CanMove[WHITE_KING][i]);
		}
	} else {
		//歩
		if (board_[to + U] == WHITE_PAWN) {
			return basicMove(to, to + U);
		}
		//香
		for (Square from = to + U; board_[from] != WALL; from = from + U) {
			//香があればよい
			if (board_[from] == WHITE_LANCE) {
				return basicMove(to, from);
			} else if (board_[from] != EMPTY) { //ほかの駒があったらダメ
				break;
			}
		}
		//桂
		if (board_[to + LUU] == WHITE_KNIGHT) {
			return basicMove(to, to + LUU);
		} else if (board_[to + RUU] == WHITE_KNIGHT) {
			return basicMove(to, to + RUU);
		}
		//銀
		for (unsigned int i = 0; i < CanMove[BLACK_SILVER].size(); i++) {
			if (board_[to + CanMove[BLACK_SILVER][i]] == WHITE_SILVER)
				return basicMove(to, to + CanMove[BLACK_SILVER][i]);
		}
		//金
		for (unsigned int i = 0; i < CanMove[BLACK_GOLD].size(); i++) {
			switch (board_[to + CanMove[BLACK_GOLD][i]]) {
			case WHITE_GOLD:
			case WHITE_PAWN_PROMOTE:
			case WHITE_LANCE_PROMOTE:
			case WHITE_KNIGHT_PROMOTE:
			case WHITE_SILVER_PROMOTE:
				return basicMove(to, to + CanMove[BLACK_GOLD][i]);
			}
		}
		//馬
		for (unsigned int i = 0; i < CanMove[BLACK_BISHOP_PROMOTE].size(); i++) {
			if (board_[to + CanMove[BLACK_BISHOP_PROMOTE][i]] == WHITE_BISHOP_PROMOTE)
				return basicMove(to, to + CanMove[BLACK_BISHOP_PROMOTE][i]);
		}
		//角と馬
		for (unsigned int i = 0; i < CanJump[BLACK_BISHOP].size(); i++) {
			for (Square from = to + CanJump[BLACK_BISHOP][i]; board_[from] != WALL; from = from + CanJump[BLACK_BISHOP][i]) {
				if (board_[from] == WHITE_BISHOP || board_[from] == WHITE_BISHOP_PROMOTE) {
					return basicMove(to, from);
				} else if (board_[from] != EMPTY) break;
			}
		}
		//竜
		for (unsigned int i = 0; i < CanMove[BLACK_ROOK_PROMOTE].size(); i++) {
			if (board_[to + CanMove[BLACK_ROOK_PROMOTE][i]] == WHITE_ROOK_PROMOTE)
				return basicMove(to, to + CanMove[BLACK_ROOK_PROMOTE][i]);
		}
		//飛車と馬
		for (unsigned int i = 0; i < CanJump[BLACK_ROOK].size(); i++) {
			for (Square from = to + CanJump[BLACK_ROOK][i]; board_[from] != WALL; from = from + CanJump[BLACK_ROOK][i]) {
				if (board_[from] == WHITE_ROOK || board_[from] == WHITE_ROOK_PROMOTE) {
					return basicMove(to, from);
				} else if (board_[from] != EMPTY) break;
			}
		}
		//玉
		for (unsigned int i = 0; i < CanMove[BLACK_KING].size(); i++) {
			if (board_[to + CanMove[BLACK_KING][i]] == WHITE_KING && control_num_[~color_][to] == 0)
				return basicMove(to, to + CanMove[BLACK_KING][i]);
		}
	}
	return basicMove(WALL00, WALL00);
}

inline bool Position::isPsuedoLegalMove(Move move)
{
	//動き方は駒の種類に即していることを前提に、いくつか簡易的なチェックをするだけ

	//移動先が盤内じゃなかったらダメ
	if (!isOnBoard(move.to())) return false;

	//移動先に手番側の駒があったらダメ
	if (pieceToColor(board_[move.to()]) == color_) return false;

	//移動元が玉位置(つまり玉を動かす手)であり、移動先に相手側の利きがあったらダメ
	if (move.from() == king_sq_[color_] && control_num_[~color_][move.to()] > 0) return false;

	//打ち歩詰めだったらダメ
	if (move.isDrop() && kind(move.subject()) == PAWN) {
		if (color_ == BLACK && move.to() == king_sq_[WHITE] + D) {
			doMove(move);
			if (generateAllMoves().size() == 0) {
				undo();
				return false;
			}
			undo();
			return true;
		} else if (color_ == WHITE && move.to() == king_sq_[BLACK] + U) {
			doMove(move);
			if (generateAllMoves().size() == 0) {
				undo();
				return false;
			}
			undo();
			return true;
		}
	}

	//駒がピンの状態にあるかどうかの判定
	//まずfromの位置に飛び利きが立っているか判定
	if (control_dir_[~color_][move.from()] & JUMP_MASK) {
		
		//立っていたら8方向について利きを見る
		for (int i = 0; i < 8; i++) {

			//fromに飛び利きがあり
			if ((control_dir_[~color_][move.from()] & JumpControlList[i])

				//自玉に同じ方向から影の利きがあり
				&& (control_dir_[~color_][king_sq_[color_]] & ShadowControlList[i])

				//利きの方向が自玉-動く駒の位置関係と同じ
				&& (directionAtoB(king_sq_[color_], move.from()) == DirList[i])) {
				
				//上記3条件を満たすとき、fromにある駒は飛び利きで釘付けされている
				//動ける方向は利きの方向、あるいはその逆方向だけ
				if (directionAtoB(move.from(), move.to()) == DirList[i] || directionAtoB(move.to(), move.from()) == DirList[i]) return true;
				else return false;
			}
		}
	}
	return true;
}

inline bool Position::canPromote(Move move)
{
	//打つ手だったらダメ
	if (move.isDrop() != EMPTY) return false;

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

void Position::pushMove(const Move move, std::vector<Move> &move_list)
{
	//簡易合法判定をする
	if (!isPsuedoLegalMove(move)) return;

	//成る手が可能だったら先に生成
	if (canPromote(move)) {
		move_list.push_back(promotiveMove(move));
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
	move_list.push_back(move);
}

void Position::pushCheckMove(const Move move, std::vector<Move> &move_list)
{
	//簡易合法判定をする
	if (!isPsuedoLegalMove(move)) return;

	//成る手が可能だったら先に生成
	if (canPromote(move)) {
		Move promotive_move = promotiveMove(move);
		doMove(promotive_move);
		if (color_ == BLACK && isKingChecked(WHITE) || color_ == WHITE && isKingChecked(BLACK))
			move_list.push_back(promotive_move);
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
	if (color_ == BLACK && isKingChecked(WHITE) || color_ == WHITE && isKingChecked(BLACK))
		move_list.push_back(move);
	undo();
}

std::vector<Move> Position::generateAllMoves()
{
	std::vector<Move> move_list;
	move_list.reserve(MAX_MOVE_LIST_SIZE);
	//手番側に王手がかかっていたら逃れる手だけを生成
	if (isKingChecked(color_)) return generateEvasionMoves();

	//簡易的なオーダリングとして取る手を先に生成する

	//盤上を走査して取る手生成
	for (Square sq : SquareList) {
		//sqに敵駒があったらそこへ動く手を生成
		if (pieceToColor(board_[sq]) == ~color_)
			addMovesTo(sq, move_list);
	}

	//盤上を走査して取る手以外生成
	for (Square sq : SquareList) {
		//sqに駒があるなら次のマスへ
		if (board_[sq] != EMPTY) continue;

		//そうでないならそこへ動くMoveと打つMoveを生成
		//動くMove
		addMovesTo(sq, move_list);
		
		//打つMove
		addDropMovesTo(sq, move_list);
	}
	/*
	//盤上を走査して合法手生成
	for (Square from : SquareList) {
		//見てるマスに手番ではない方の駒があったら関係ないので次のマスへ
		if (pieceToColor(board_[from]) == ~color_) continue;

		//動かす駒
		Piece subject = board_[from];

		//fromが空だったらそこへ打つ手を生成
		if (subject == EMPTY)  addDropMovesTo(from, move_list);
		else { //駒があったら動かす手を生成
			//隣接および桂馬の動き
			for (unsigned int i = 0; i < CanMove[subject].size(); i++) {
				pushMove(basicMove(from + CanMove[subject][i], from), move_list);
			}
			//飛ぶ動き
			for (unsigned int i = 0; i < CanJump[subject].size(); i++) {
				for (Square to = from + CanJump[subject][i]; board_[to] != WALL && !(pieceToColor(board_[to]) == color_); to = to + CanJump[subject][i]) {
					pushMove(basicMove(to, from), move_list);
					if (pieceToColor(board_[to]) == ~color_) break;
				}
			}
		}
	}
	*/
	return move_list;
}

std::vector<Move> Position::generateEvasionMoves()
{
	//玉位置への利きを読み取って逃れる手を生成する
	//王手としてありえるのは飛び利き2つの両王手、飛び利きと直接利きの両王手、飛び利きの王手、直接利きの王手という4パターン
	//王手への対応は(a)玉が逃げる、(b)王手してきた駒を取る、(c)飛び利きの王手には合駒

	std::vector<Move> evasion_move_list;

	//手番のほうの玉の位置を設定
	Square evasion_from = king_sq_[color_];

	//(a)逃げる手
	//隣接王手を玉で取り返せる場合はここに含む
	for (int i = 0; i < 8; i++) {
		Square evasion_to = king_sq_[color_] + DirList[i];

		//逃げる方向と逆から飛び利きが来ていたらその方向には逃げられない
		if (control_dir_[~color_][king_sq_[color_]] & DirToOppositeJumpControl[DirList[i]]) continue;

		//移動先に手番でないほうの利きがあってはいけない
		if (control_num_[~color_][evasion_to]) continue;

		//移動先に手番側の駒があってはいけない
		if (pieceToColor(board_[evasion_to]) == color_) continue;

		//移動先は盤上でなくてはならない
		if (!isOnBoard(evasion_to)) continue;

		//上記条件を満たしたものをリストに加える
		evasion_move_list.push_back(basicMove(evasion_to, evasion_from));
	}

	//(b)王手してきた駒を玉以外で取る手
	//両王手だったら無理なので直接利きと飛び利きだけ取り出してみて立ってるビット数が1であることを確認する
	if (__popcnt(control_dir_[~color_][king_sq_[color_]] & (DIR_MASK | JUMP_MASK)) == 1) {
		//8方向についてはループを回す
		for (int i = 0; i < 8; i++) {
			if (DirectControlList[i] & control_dir_[~color_][king_sq_[color_]]) {
				addMovesTo(king_sq_[color_] + DirList[i], evasion_move_list);
				break; //立っている利きは1つだからbreakしていい
			}
			if (JumpControlList[i] & control_dir_[~color_][king_sq_[color_]]) {
				for (Square to = king_sq_[color_] + DirList[i]; board_[to] != WALL; to = to + DirList[i]) {
					if (board_[to] != EMPTY) { //駒に突き当たったらその駒を取る動きを生成
						addMovesTo(to, evasion_move_list);
						break;
					}
				}
				break; //立っている利きは1つだからbreakしていい
			}
		}
		//桂馬による王手は別にみる
		for (unsigned int i = 0; i < 2; i++) {
			//手番側の玉位置に桂馬の利きが立っているか
			if (DirToControlDir[CanMove[static_cast<Piece>(ColorToFlag[color_] + KNIGHT)][i]] & control_dir_[~color_][king_sq_[color_]]) {
				addMovesTo(king_sq_[color_] + CanMove[static_cast<Piece>(ColorToFlag[color_] + KNIGHT)][i], evasion_move_list);
			}
		}
	}

	//(c)合駒
	//飛び利きしか無理 && 両王手だったら無理なので飛び利きだけ取り出してみて立ってるビット数が1であることを確認する
	if (__popcnt(control_dir_[~color_][king_sq_[color_]] & JUMP_MASK) == 1
		&& __popcnt(control_dir_[~color_][king_sq_[color_]] & DIR_MASK) == 0) {
		//周囲8方向を順番に見ていく	
		for (int i = 0; i < 8; i++) {
			//手番側の玉位置に飛び利きが立っているか
			if (control_dir_[~color_][king_sq_[color_]] & JumpControlList[i]) {
				//玉と飛び駒の間を1マスずつ見ていって合駒する手を生成
				for (Square to = king_sq_[color_] + DirList[i]; board_[to] == EMPTY; to = to + DirList[i]) {
					//駒打ちでの合駒
					addDropMovesTo(to, evasion_move_list);
					//移動合
					addMovesTo(to, evasion_move_list);
				}
				//飛び利きは1つのはずなので見つかったらbreak
				break;
			}
		}
	}
	return evasion_move_list;
}

void Position::addMovesTo(Square to, std::vector<Move> &move_list)
{
	//toの利きを見てtoへ動くMoveを生成する

	//移動先に手番側の駒があったらダメ
	if (pieceToColor(board_[to]) == color_) return;

	//手番側のtoへの利き、control_dir_[color_][to]を見る
	//周囲8方向についてはfromを回してみる
	for (int i = 0; i < 8; i++) {
		//周囲8方向の直接利き
		if (control_dir_[color_][to] & DirectControlList[i]) {
			Square from = to + DirList[i];
			//手番側の玉が相手側の利きに飛び込むようなMoveはダメなのでそれ以外のとき指し手を追加
			if (!(from == king_sq_[color_] && control_num_[~color_][to] > 0))
				pushMove(basicMove(to, from), move_list);
		}
		//周囲8方向の飛び利き
		if (control_dir_[color_][to] & JumpControlList[i]) {
			//利きがある方向に1マスずつ駒に突き当るまで辿っていく
			for (Square from = to + DirList[i]; board_[from] != WALL; from = from + DirList[i]) {
				if (board_[from] != EMPTY) {
					pushMove(basicMove(to, from), move_list);
					break;
				}
			}
		}
	}
	//桂馬は別にみる
	//toへ手番側の桂馬が利いている == (to + 手番じゃないほうの桂馬の動けるマス)に手番側の桂馬がある
	for (unsigned int i = 0; i < 2; i++) {
		//toの位置への手番側の利きとして逆利きがあるか
		if (DirToControlDir[CanMove[static_cast<Piece>(ColorToFlag[~color_] + KNIGHT)][i]] & control_dir_[color_][to]) {
			Square from = to + CanMove[static_cast<Piece>(ColorToFlag[~color_] + KNIGHT)][i];
			pushMove(basicMove(to, from), move_list);
		}
	}
}

void Position::addDropMovesTo(Square to, std::vector<Move> &move_list)
{
	if (color_ == BLACK) {
		if (SquareToRank[to] >= Rank2) {
			//歩
			if (hand_[BLACK].num(PAWN) > 0 && canDropPawn<BLACK>(SquareToFile[to])) {
				if (board_[to + U] == WHITE_KING) { //打ち歩詰めになってないか確認
					doMove(dropMove(to, BLACK_PAWN));
					if (generateAllMoves().size() == 0) {
						undo();
						return;
					}
					undo();
				}
				move_list.push_back(dropMove(to, BLACK_PAWN));
			}
			//香
			if (hand_[BLACK].num(LANCE) > 0) move_list.push_back(dropMove(to, BLACK_LANCE));
		}
		if (SquareToRank[to] >= Rank3) {
			//桂
			if (hand_[BLACK].num(KNIGHT) > 0) move_list.push_back(dropMove(to, BLACK_KNIGHT));
		}
		//その他
		for (Piece p = SILVER; p <= ROOK; p++) {
			if (hand_[BLACK].num(p) > 0) move_list.push_back(dropMove(to, toBlack(p)));
		}
	} else {
		if (SquareToRank[to] <= Rank8) {
			//歩
			if (hand_[WHITE].num(PAWN) > 0 && canDropPawn<WHITE>(SquareToFile[to])) {
				if (board_[to + D] == BLACK_KING) { //打ち歩詰めになってないか確認
					doMove(dropMove(to, WHITE_PAWN));
					if (generateAllMoves().size() == 0) {
						undo();
						return;
					}
					undo();
				}
				move_list.push_back(dropMove(to, WHITE_PAWN));
			}
			//香
			if (hand_[WHITE].num(LANCE) > 0) move_list.push_back(dropMove(to, WHITE_LANCE));
		}
		if (SquareToRank[to] <= Rank7) {
			//桂
			if (hand_[WHITE].num(KNIGHT) > 0) move_list.push_back(dropMove(to, WHITE_KNIGHT));
		}
		//その他
		for (Piece p = SILVER; p <= ROOK; p++) {
			if (hand_[WHITE].num(p) > 0) move_list.push_back(dropMove(to, toWhite(p)));
		}
	}
}

void Position::addCaptureMoves(std::vector<Move> &move_list)
{
	//盤上を走査して取る手生成
	for (Square sq : SquareList) {
		//sqに敵駒があったらそこへ動く手を生成
		if (pieceToColor(board_[sq]) == ~color_)
			addMovesTo(sq, move_list);
	}
}

void Position::addNonCaptureMoves(std::vector<Move> &move_list)
{
	//盤上を走査して取る手生成
	for (Square sq : SquareList) {
		//sqに駒があるなら次のマスへ
		if (board_[sq] != EMPTY) continue;

		//そうでないならそこへ動くMoveと打つMoveを生成
		addMovesTo(sq, move_list);
		addDropMovesTo(sq, move_list);
	}
}

