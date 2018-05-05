#ifndef POSITION_HPP
#define POSITION_HPP

#include"square.hpp"
#include"piece.hpp"
#include"move.hpp"
#include"hand.hpp"
#include"eval_elements.hpp"
#include"types.hpp"
#include"eval_elements.hpp"
#include"bitboard.hpp"
#include"control.hpp"
#include<random>
#include<cstdint>
#include<unordered_map>

const int MAX_MOVE_LIST_SIZE = 593;

class Position {
public:
    //コンストラクタ
    Position();

    //入出力
    void print() const;
    void printAllMoves() const;
    void printHistory() const;
    void printForDebug() const;
    void printScoreDetail() const;

    //動かす戻す
    void doMove(const Move move);
    void undo();
    void doNullMove();
    void undoNullMove();

    //合法性に関するもの
    bool isLegalMove(const Move move) const;
    bool canDropPawn(const Square to) const;
    bool isPsuedoLegalMove(const Move move) const;

    //王手,詰み関連
    inline bool isKingChecked() const { return isChecked_; }
    inline bool isCheckmated() { return (generateAllMoves().size() == 0); }
    inline bool isThereControl(const Color c, const Square sq) const { return controls_[c][sq].number > 0; }
    bool isRepeating(Score& score);

    //評価値計算
    virtual Score initScore();
    virtual Score calcScoreDiff();
    void testPieceValue();

    //特徴量作成
    Features makeEvalElements() const;
    Features makeEvalElements(std::vector<Move>& pv, Color& leaf_color);

    //Moveを完全にする
    Move transformValidMove(const Move move);

    Bitboard attackersTo(const Color c, const Square sq) const;
    Bitboard attackersTo(const Color c, const Square sq, const Bitboard& occupied) const;

    void computePinners();

    //合法手生成
    //ステージングに関わるもの
    std::vector<Move> generateAllMoves() const;
    void generateCheckMoveBB(Move*& move_ptr) const;
    void generateEvasionMovesBB(Move*& move_ptr) const;
    void generateCaptureMovesBB(Move*& move_ptr) const;
    void generateNonCaptureMovesBB(Move*& move_ptr) const;
    void generateRecaptureMovesToBB(const Square to, Move*& move_ptr) const;

    void generatePsuedoCaptureMoves(Move*& move_ptr) const;
    void generatePsuedoNonCaptureMoves(Move*& move_ptr) const;
    void generatePsuedoRecaptureMovesTo(const Square to, Move*& move_ptr) const;

    //部分に関わるもの
    bool canPromote(const Move move) const;
    void pushMove(const Move move, Move*& move_ptr) const;
    void pushCheckMove(const Move move, Move*& move_ptr);

    void generateDropMoveBB(const Bitboard& to_bb, Move*& move_ptr) const;

    bool isMoreControl(const Square to) const;

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
    Score score() const;
    Piece on(const Square sq) const { return board_[sq]; }
protected:
	//内部メソッド
	//controlsを操作するメソッド
	void putPiece(Piece p,Square sq);
	void removePiece(Piece p,Square sq);
	void changeControl(Piece p, Square sq, int diff);
	void changeLongControl(Square sq, ControlDir cd, Color c, int extend_or_block);
    void initControl();
    void checkControl();
    void printControl() const;

	//インスタンス変数類
    //手番
    Color color_;
    Piece board_[SquareNum];
    Hand hand_[ColorNum];
    unsigned int turn_number_;
    Score piece_score_, kp_score_, pp_score_;
    Square king_sq_[ColorNum];
    std::vector<Move> kifu_;
    long long HashSeed[PieceNum][SquareNum];
    long long HandHashSeed[ColorNum][PieceNum][19];
    long long hash_value_, board_hash_, hand_hash_;
    bool isChecked_;

    struct StateInfo {
        //千日手判定用に必要な情報
        long long board_hash, hand_hash;
        bool isChecked;

        //undoでコピーするための情報
        Score piece_score, kp_score, pp_score;
        Features features;

        Bitboard pinners;

        StateInfo(Position& pos) :
            board_hash(pos.board_hash_), hand_hash(pos.hand_hash_), isChecked(pos.isChecked_), piece_score(pos.piece_score_),
            kp_score(pos.kp_score_), pp_score(pos.pp_score_), features(pos.ee_), pinners(pos.pinners_) {
        }
    };
    std::vector<StateInfo> stack_;

    Bitboard occupied_all_;
    Bitboard occupied_bb_[ColorNum];
    Bitboard pieces_bb_[PieceNum];
    Bitboard pinners_;

    Control controls_[ColorNum][SquareNum];

    Features ee_;
};

#endif