#ifndef POSITION_HPP
#define POSITION_HPP

#include"square.hpp"
#include"piece.hpp"
#include"move.hpp"
#include"hand.hpp"
#include"types.hpp"
#include<random>
#include<cstdint>

const int MAX_MOVE_LIST_SIZE = 593;

class Position {
public:
	//コンストラクタ
	Position();
	
	//入出力
	void print() const;
	void printControlDir() const;
	void printAllMoves();
	void printHistory()const;

	//動かす戻す
	void doMove(Move move);
	void undo();
    void doNullMove();
    void undoNullMove();

	//合法性に関するもの
	bool isLegalMove(const Move move) const;
	template<Color color> bool canDropPawn(const File f) const;
	bool isLegalControl();
    bool isPsuedoLegalMove(const Move move) const;

	//王手,詰み関連
	inline bool isKingChecked(const Color color) const { return (control_num_[~color][king_sq_[color]] > 0); }
	inline bool isCheckmated() { return (generateAllMoves().size() == 0); }
    bool isRepeating(Score& score);

	//利き
	void initControl();
	void deleteControl(const Square sq);
	void addControl(const Square sq);
	void addJumpControl(const Color BorW, const Square sq, const ControlDir dir);

	//評価値計算
	Score initScore();
	void testPieceValue();

    //合法手生成
    //ステージングに関わるもの
	std::vector<Move> generateAllMoves();
    void generateCheckMoves(Move*& move_ptr);
	void generateEvasionMoves(Move*& move_ptr) const;
	void generateCaptureMoves(Move*& move_ptr) const;
	void generateNonCaptureMoves(Move*& move_ptr) const;
    void generateRecaptureMovesTo(const Square to, Move*& move_ptr) const;

	//部分に関わるもの
    void addMovesTo(const Square to, Move*& move_ptr) const;
	void addDropMovesTo(const Square to, Move*& move_ptr) const;
	bool canPromote(const Move move) const;
	void pushMove(const Move move, Move*& move_ptr) const;
	void pushCheckMove(const Move move, Move*& move_ptr);

    //sfen読み込み
    void load_sfen(std::string sfen);

    //ハッシュ
    void hashInit();
	
	//getter
	Move lastMove() const { return kifu_.back(); }
	unsigned int turn_number() const { return turn_number_; }
	Color color() const { return color_; }
	long long hash_value() const { return hash_value_; }
	Square kingSquare(Color color) const { return king_sq_[color]; }
    Score score();
    Piece on(Square sq) const { return board_[sq]; }

private:
    //手番
	Color color_;
	Piece board_[SquareNum];
	Hand hand_[ColorNum];
	unsigned int turn_number_;
	unsigned int control_num_[ColorNum][SquareNum];
	unsigned int control_dir_[ColorNum][SquareNum];	
    Score score_, piece_score_, control_score_, control_num_score_;
	Square king_sq_[ColorNum];
	std::vector<Move> kifu_;
	long long HashSeed[PieceNum][SquareNum];
	long long HandHashSeed[ColorNum][PieceNum][19];
	long long hash_value_, board_hash_, hand_hash_;

    struct StateInfo {
        //千日手判定用に必要な情報だけ保持しておく
        long long board_hash, hand_hash;
        bool isChecked;

        StateInfo(long long bh, long long hh, bool b) :
            board_hash(bh), hand_hash(hh), isChecked(b) {}
    };
    std::vector<StateInfo> stack_;
};

#endif