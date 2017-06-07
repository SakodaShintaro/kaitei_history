#ifndef POSITION_HPP
#define POSITION_HPP

#include"square.hpp"
#include"piece.hpp"
#include"move.hpp"
#include"hand.hpp"
#include<random>
#include<map>

const int MAX_MOVE_LIST_SIZE = 593;

class Position {
public:
    //コンストラクタ
    Position();

    //入出力
    void print() const;
    void printCheckMoves();
    void printControlDir();
    void printAllMoves();
    void printCaptureMoves();
    void printNonCaptureMoves();
    void printHistory()const;

    //動かす戻す
    void doMove(Move move);
    void undo();
    //合法性に関するもの
    bool isLegalMove(const Move move);
    template<Color color> bool canDropPawn(const File f);
    bool isLegalControl();

    //王手,詰み関連
    inline bool isKingChecked(const Color color) const { return (control_num_[~color][king_sq_[color]] > 0); }
    inline bool isCheckmated() { return (generateAllMoves().size() == 0); }

    //利き
    void initControl();
    void deleteControl(Square sq);
    void addControl(Square sq);
    void blockJumpControl(Color BorW, Square sq, ControlDir dir);
    void blockShadowControl(Color BorW, Square sq, ControlDir dir);
    void extendJumpControl(Color BorW, Square sq, ControlDir dir);
    void extendShadowControl(Color BorW, Square sq, ControlDir dir);
    void addJumpControl(Color BorW, Square sq, ControlDir dir);
    void removeJumpControl(Color BorW, Square sq, ControlDir dir);

    //評価値計算
    int initScore();
    void testPieceValue();

    //合法手生成
    std::vector<Move> generateAllMoves();
    std::vector<Move> generateCheckMoves();
    std::vector<Move> generateEvasionMoves();
    void addMovesTo(Square to, std::vector<Move> &moves);
    void addDropMovesTo(Square to, std::vector<Move> &moves);
    void addCaptureMoves(std::vector<Move> &moves);
    void addNonCaptureMoves(std::vector<Move> &moves);
    Move generateCheapestMoveTo(Square pos);
    bool isPsuedoLegalMove(Move move);
    bool canPromote(Move move);
    void pushMove(const Move move, std::vector<Move> &move_list);
    void pushCheckMove(const Move move, std::vector<Move> &move_list);

    //sfen読み込み
    void load_sfen(std::string sfen);
    //ハッシュ
    void hashInit();
    //getter
    Move lastMove() const { return kifu_.back(); }
    unsigned int turn_number() const { return turn_number_; }
    Color color() const { return color_; }
    long long hash_val() const { return hash_val_; }
    Square kingSquare(Color color) const { return king_sq_[color]; }
    int score() const { return score_; }
private:
    Color color_;
    Piece board_[SquareNum];
    Hand hand_[ColorNum];
    unsigned int turn_number_;
    unsigned int control_num_[ColorNum][SquareNum];
    unsigned int control_dir_[ColorNum][SquareNum];
    int score_;
    int piece_score_;
    int control_score_;
    int control_num_score_;
    Square king_sq_[ColorNum];
    std::vector<Move> kifu_;
    long long HashSeed[PieceNum][SquareNum];
    long long HandHashSeed[ColorNum][PieceNum][19];
    long long hash_val_;
    std::map<long long, int> num_occurrences;
};

#endif
